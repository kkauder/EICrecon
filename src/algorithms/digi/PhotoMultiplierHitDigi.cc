// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2022, 2023, Chao Peng, Thomas Britton, Christopher Dilks, Luigi Dello Stritto

/*  General PhotoMultiplier Digitization
 *
 *  Apply the given quantum efficiency for photon detection
 *  Converts the number of detected photons to signal amplitude
 *
 *  Author: Chao Peng (ANL)
 *  Date: 10/02/2020
 *
 *  Ported from Juggler by Thomas Britton (JLab)
 */


#include "PhotoMultiplierHitDigi.h"

#include <JANA/JEvent.h>

//------------------------
// AlgorithmInit
//------------------------
void eicrecon::PhotoMultiplierHitDigi::AlgorithmInit(dd4hep::Detector *detector, std::shared_ptr<spdlog::logger>& logger)
{
    // services
    m_detector = detector;
    m_cellid_converter = std::make_shared<const dd4hep::rec::CellIDPositionConverter>(*detector);
    m_log=logger;

    // print the configuration parameters
    m_cfg.Print(m_log, spdlog::level::debug);

    /* warn if using potentially thread-unsafe seed
     * FIXME: remove this warning when this issue is resolved:
     *        https://github.com/eic/EICrecon/issues/539
     */
    if(m_cfg.seed==0) m_log->warn("using seed=0 may cause thread-unsafe behavior of TRandom (EICrecon issue 539)");

    // random number generators
    m_random.SetSeed(m_cfg.seed);
    m_rngNorm = [&](){
        return m_random.Gaus(0., 1.0);
    };
    m_rngUni = [&](){
        return m_random.Uniform(0., 1.0);
    };
    //auto randSvc = svc<IRndmGenSvc>("RndmGenSvc", true);
    auto sc1 = m_rngUni;//m_rngUni.initialize(randSvc, Rndm::Flat(0., 1.));
    auto sc2 = m_rngNorm;//m_rngNorm.initialize(randSvc, Rndm::Gauss(0., 1.));
    //if (!sc1.isSuccess() || !sc2.isSuccess()) {
    if (!sc1 || !sc2) {
        throw std::runtime_error("Cannot initialize random generator!");
    }

    // initialize quantum efficiency table
    qe_init();
}



//------------------------
// AlgorithmChangeRun
//------------------------
void eicrecon::PhotoMultiplierHitDigi::AlgorithmChangeRun() {
    /// This is automatically run before Process, when a new run number is seen
    /// Usually we update our calibration constants by asking a JService
    /// to give us the latest data for this run number
}

//------------------------
// AlgorithmProcess
//------------------------
eicrecon::PhotoMultiplierHitDigiResult eicrecon::PhotoMultiplierHitDigi::AlgorithmProcess(
    const edm4hep::SimTrackerHitCollection* sim_hits
    )
{
        m_log->trace("{:=^70}"," call PhotoMultiplierHitDigi::AlgorithmProcess ");
        std::unordered_map<CellIDType, std::vector<HitData>> hit_groups;
        // collect the photon hit in the same cell
        // calculate signal
        m_log->trace("{:-<70}","Loop over simulated hits ");
        for(std::size_t sim_hit_index = 0; sim_hit_index < sim_hits->size(); sim_hit_index++) {
            const auto& sim_hit = sim_hits->at(sim_hit_index);
            auto edep_eV = sim_hit.getEDep() * 1e9; // [GeV] -> [eV] // FIXME: use common unit converters, when available
            auto id      = sim_hit.getCellID();
            m_log->trace("hit: pixel id={:#018X}  edep = {} eV", id, edep_eV);

            // overall safety factor
            if (m_rngUni() > m_cfg.safetyFactor) continue;

            // quantum efficiency
            if (!qe_pass(edep_eV, m_rngUni())) continue;

            // pixel gap cuts
            // FIXME: generalize; this assumes the segmentation is `CartesianGridXY`
            dd4hep::Position pos_pixel, pos_hit, pos_hit_global;
            if(m_cfg.enablePixelGaps) {
              pos_hit_global =  m_cellid_converter->position(id);
              pos_pixel = get_sensor_local_position(id, pos_hit_global);
              pos_hit   = get_sensor_local_position(id, vec2pos(sim_hit.getPosition()));
              if( std::abs( pos_hit.x()/dd4hep::mm - pos_pixel.x()/dd4hep::mm ) > m_cfg.pixelSize/2 ||
                  std::abs( pos_hit.y()/dd4hep::mm - pos_pixel.y()/dd4hep::mm ) > m_cfg.pixelSize/2
                ) continue;
            }

            // cell time, signal amplitude, truth photon
            m_log->trace(" -> hit accepted");
            m_log->trace(" -> MC hit id={}", sim_hit.id());
            auto   time = sim_hit.getTime();
            double amp  = m_cfg.speMean + m_rngNorm() * m_cfg.speError;

            // insert hit to `hit_groups`
            InsertHit(
                hit_groups,
                id,
                amp,
                time,
                pos_hit,
                pos_hit_global,
                sim_hit_index
                );
        }

        // print `hit_groups`
        if(m_log->level() <= spdlog::level::trace) {
          m_log->trace("{:-<70}","Accepted hit groups ");
          for(auto &[id,hitVec] : hit_groups)
            for(auto &hit : hitVec) {
              m_log->trace("hit_group: pixel id={:#018X} -> npe={} signal={} time={}", id, hit.npe, hit.signal, hit.time);
              for(auto i : hit.sim_hit_indices)
                m_log->trace(" - MC hit: EDep={}, id={}", sim_hits->at(i).getEDep(), sim_hits->at(i).id());
            }
        }

        //build noise raw hits
        if (m_cfg.enableNoise) {
          m_log->trace("{:=^70}"," BEGIN NOISE INJECTION ");
          float p = m_cfg.noiseRate*m_cfg.noiseTimeWindow;
          auto cellID_action = [this,&hit_groups] (auto id) {

            // cell time, signal amplitude
            double   amp  = m_cfg.speMean + m_rngNorm()*m_cfg.speError;
            TimeType time = m_cfg.noiseTimeWindow*m_rngUni() / dd4hep::ns;
            dd4hep::Position pos_hit_global = m_cellid_converter->position(id);

            // insert in `hit_groups`, or if the pixel already has a hit, update `npe` and `signal`
            this->InsertHit(
                hit_groups,
                id,
                amp,
                time,
                dd4hep::Position{0,0,0}, // not used
                pos_hit_global,
                0, // not used
                true
                );

          };
          m_VisitRngCellIDs(cellID_action, p);
        }

        // build output `RawTrackerHit` and `MCRecoTrackerHitAssociation` collections
        m_log->trace("{:-<70}","Digitized raw hits ");
        PhotoMultiplierHitDigiResult result;
        result.raw_hits   = std::make_unique<edm4eic::RawTrackerHitCollection>();
        result.hit_assocs = std::make_unique<edm4eic::MCRecoTrackerHitAssociationCollection>();
        for (auto &it : hit_groups) {
            for (auto &data : it.second) {

                // build `RawTrackerHit`
                auto raw_hit = result.raw_hits->create();
                raw_hit.setCellID(it.first);
                raw_hit.setCharge(    static_cast<decltype(edm4eic::RawTrackerHitData::charge)>    (data.signal)                    );
                raw_hit.setTimeStamp( static_cast<decltype(edm4eic::RawTrackerHitData::timeStamp)> (data.time/m_cfg.timeResolution) );
                // raw_hit.setPosition(pos2vec(data.pos)) // TEST gap cuts; FIXME: requires member `edm4hep::Vector3d position`
                                                          // in data model datatype, think of a better way
                m_log->trace("raw_hit: cellID={:#018X} -> charge={} timeStamp={}",
                    raw_hit.getCellID(),
                    raw_hit.getCharge(),
                    raw_hit.getTimeStamp()
                    );

                // build `MCRecoTrackerHitAssociation` (for non-noise hits only)
                if(!data.sim_hit_indices.empty()) {
                  auto hit_assoc = result.hit_assocs->create();
                  hit_assoc.setWeight(1.0); // not used
                  hit_assoc.setRawHit(raw_hit);
                  for(auto i : data.sim_hit_indices)
                    hit_assoc.addToSimHits(sim_hits->at(i));
                }
            }
        }
        return result;
}

void  eicrecon::PhotoMultiplierHitDigi::qe_init()
{
        // get quantum efficiency table
        qeff.clear();
        auto hc = dd4hep::h_Planck * dd4hep::c_light / (dd4hep::eV * dd4hep::nm); // [eV*nm]
        for(const auto &[wl,qe] : m_cfg.quantumEfficiency) {
          qeff.emplace_back( hc / wl, qe ); // convert wavelength [nm] -> energy [eV]
        }

        // sort quantum efficiency data first
        std::sort(qeff.begin(), qeff.end(),
            [] (const std::pair<double, double> &v1, const std::pair<double, double> &v2) {
                return v1.first < v2.first;
            });

        // print the table
        m_log->debug("{:-^60}"," Quantum Efficiency vs. Energy ");
        for(auto& [en,qe] : qeff)
          m_log->debug("  {:>10.4} {:<}",en,qe);
        m_log->trace("{:=^60}","");

        // sanity checks
        if (qeff.empty()) {
            qeff = {{2.6, 0.3}, {7.0, 0.3}};
            m_log->warn("Invalid quantum efficiency data provided, using default values {} {:.2f} {} {:.2f} {} {:.2f} {} {:.2f} {}","{{", qeff.front().first, ",", qeff.front().second, "},{",qeff.back().first,",",qeff.back().second,"}}");
        }
        if (qeff.front().first > 3.0) {
            m_log->warn("Quantum efficiency data start from {:.2f} {}", qeff.front().first, " eV, maybe you are using wrong units?");
        }
        if (qeff.back().first < 3.0) {
            m_log->warn("Quantum efficiency data end at {:.2f} {}", qeff.back().first, " eV, maybe you are using wrong units?");
        }
}


template<class RndmIter, typename T, class Compare> RndmIter  eicrecon::PhotoMultiplierHitDigi::interval_search(RndmIter beg, RndmIter end, const T &val, Compare comp) const
{
        // special cases
        auto dist = std::distance(beg, end);
        if ((dist < 2) || (comp(*beg, val) > 0) || (comp(*std::prev(end), val) < 0)) {
            return end;
        }
        auto mid = std::next(beg, dist / 2);

        while (mid != end) {
            if (comp(*mid, val) == 0) {
                return mid;
            } else if (comp(*mid, val) > 0) {
                end = mid;
            } else {
                beg = std::next(mid);
            }
            mid = std::next(beg, std::distance(beg, end)/2);
        }

        if (mid == end || comp(*mid, val) > 0) {
            return std::prev(mid);
        }
        return mid;
}

bool  eicrecon::PhotoMultiplierHitDigi::qe_pass(double ev, double rand) const
{
        auto it = interval_search(qeff.begin(), qeff.end(), ev,
                    [] (const std::pair<double, double> &vals, double val) {
                        return vals.first - val;
                    });

        if (it == qeff.end()) {
            // m_log->warn("{} eV is out of QE data range, assuming 0\% efficiency",ev);
            return false;
        }

        double prob = it->second;
        auto itn = std::next(it);
        if (itn != qeff.end() && (itn->first - it->first != 0)) {
            prob = (it->second*(itn->first - ev) + itn->second*(ev - it->first)) / (itn->first - it->first);
        }

        // m_log->trace("{} eV, QE: {}\%",ev,prob*100.);
        return rand <= prob;
}


// transform global position `pos` to sensor `id` frame position
// IMPORTANT NOTE: this has only been tested for the dRICH; if you use it, test it carefully...
// FIXME: here be dragons...
dd4hep::Position eicrecon::PhotoMultiplierHitDigi::get_sensor_local_position(uint64_t id, dd4hep::Position pos) {

  // get the VolumeManagerContext for this sensitive detector
  auto context = m_cellid_converter->findContext(id);

  // transformation vector buffers
  double xyz_l[3], xyz_e[3], xyz_g[2];
  double pv_g[3], pv_l[3];

  // get sensor position w.r.t. its parent
  auto sensor_elem = context->element;
  sensor_elem.placement().position().GetCoordinates(xyz_l);

  // convert sensor position to global position (cf. `CellIDPositionConverter::positionNominal()`)
  const auto& volToElement = context->toElement();
  volToElement.LocalToMaster(xyz_l, xyz_e);
  const auto& elementToGlobal = sensor_elem.nominal().worldTransformation();
  elementToGlobal.LocalToMaster(xyz_e, xyz_g);
  dd4hep::Position pos_sensor;
  pos_sensor.SetCoordinates(xyz_g);

  // get the position vector of `pos` w.r.t. the sensor position `pos_sensor`
  dd4hep::Direction pos_pv = pos - pos_sensor;

  // then transform it to the sensor's local frame
  pos_pv.GetCoordinates(pv_g);
  volToElement.MasterToLocalVect(pv_g, pv_l);
  dd4hep::Position pos_transformed;
  pos_transformed.SetCoordinates(pv_l);

  // trace log
  /*
  if(m_log->level() <= spdlog::level::trace) {
    m_log->trace("pixel hit on cellID={:#018x}",id);
    auto print_pos = [&] (std::string name, dd4hep::Position p) {
      m_log->trace("  {:>30} x={:.2f} y={:.2f} z={:.2f} [mm]: ", name, p.x()/dd4hep::mm,  p.y()/dd4hep::mm,  p.z()/dd4hep::mm);
    };
    print_pos("input position",  pos);
    print_pos("sensor position", pos_sensor);
    print_pos("output position", pos_transformed);
    // auto dim = m_cellid_converter->cellDimensions(id);
    // for (std::size_t j = 0; j < std::size(dim); ++j)
    //   m_log->trace("   - dimension {:<5} size: {:.2}",  j, dim[j]);
  }
  */

  return pos_transformed;
}


// add a hit to local `hit_groups` data structure
void eicrecon::PhotoMultiplierHitDigi::InsertHit(
    std::unordered_map<CellIDType, std::vector<HitData>> &hit_groups,
    CellIDType       id,
    double           amp,
    TimeType         time,
    dd4hep::Position pos_hit_local,
    dd4hep::Position pos_hit_global,
    std::size_t      sim_hit_index,
    bool             is_noise_hit
    )
{
  auto it = hit_groups.find(id);
  if (it != hit_groups.end()) {
    std::size_t i = 0;
    for (auto ghit = it->second.begin(); ghit != it->second.end(); ++ghit, ++i) {
      if (std::abs(time - ghit->time) <= (m_cfg.hitTimeWindow)) {
        // hit group found, update npe, signal, and list of MC hits
        ghit->npe += 1;
        ghit->signal += amp;
        if(!is_noise_hit) ghit->sim_hit_indices.push_back(sim_hit_index);
        m_log->trace(" -> add to group @ {:#018X}: signal={}", id, ghit->signal);
        break;
      }
    }
    // no hits group found
    if (i >= it->second.size()) {
      auto sig = amp + m_cfg.pedMean + m_cfg.pedError * m_rngNorm();
      decltype(HitData::sim_hit_indices) indices;
      if(!is_noise_hit) indices.push_back(sim_hit_index);
      hit_groups.insert({ id, {HitData{1, sig, time, pos_hit_local, pos_hit_global, indices}} });
      m_log->trace(" -> no group found,");
      m_log->trace("    so new group @ {:#018X}: signal={}", id, sig);
    }
  } else {
    auto sig = amp + m_cfg.pedMean + m_cfg.pedError * m_rngNorm();
    decltype(HitData::sim_hit_indices) indices;
    if(!is_noise_hit) indices.push_back(sim_hit_index);
    hit_groups.insert({ id, {HitData{1, sig, time, pos_hit_local, pos_hit_global, indices}} });
    m_log->trace(" -> new group @ {:#018X}: signal={}", id, sig);
  }
}

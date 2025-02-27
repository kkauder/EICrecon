// Copyright (C) 2022, 2023, Christopher Dilks
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "IrtGeoDRICH.h"

void richgeo::IrtGeoDRICH::DD4hep_to_IRT() {

  // begin envelope
  /* FIXME: have no connection to GEANT G4LogicalVolume pointers; however all is needed
   * is to make them unique so that std::map work internally; resort to using integers,
   * who cares; material pointer can seemingly be '0', and effective refractive index
   * for all radiators will be assigned at the end by hand; FIXME: should assign it on
   * per-photon basis, at birth, like standalone GEANT code does;
   */
  auto nSectors       = m_det->constant<int>("DRICH_num_sectors");
  auto vesselZmin     = m_det->constant<double>("DRICH_zmin") / dd4hep::mm;
  auto gasvolMaterial = m_det->constant<std::string>("DRICH_gasvol_material");
  TVector3 normX(1, 0,  0); // normal vectors
  TVector3 normY(0, -1, 0);
  m_surfEntrance = new FlatSurface(TVector3(0, 0, vesselZmin), normX, normY);
  for (int isec=0; isec<nSectors; isec++) {
    auto cv = m_irtDetectorCollection->SetContainerVolume(
        m_irtDetector,              // Cherenkov detector
        RadiatorName(kGas).c_str(), // name
        isec,                       // path
        (G4LogicalVolume*)(0x0),    // G4LogicalVolume (inaccessible? use an integer instead)
        nullptr,                    // G4RadiatorMaterial (inaccessible?)
        m_surfEntrance              // surface
        );
    cv->SetAlternativeMaterialName(gasvolMaterial.c_str());
  }

  // photon detector
  // - FIXME: args (G4Solid,G4Material) inaccessible?
  auto cellMask = uint64_t(std::stoull(m_det->constant<std::string>("DRICH_cell_mask")));
  m_irtPhotonDetector = new CherenkovPhotonDetector(nullptr, nullptr);
  m_irtDetector->SetReadoutCellMask(cellMask);
  m_irtDetectorCollection->AddPhotonDetector(
      m_irtDetector,      // Cherenkov detector
      nullptr,            // G4LogicalVolume (inaccessible?)
      m_irtPhotonDetector // photon detector
      );
  m_log->debug("cellMask = {:#X}", cellMask);

  // aerogel + filter
  /* AddFlatRadiator will create a pair of flat refractive surfaces internally;
   * FIXME: should make a small gas gap at the upstream end of the gas volume;
   * FIXME: do we need a sector loop?
   * FIXME: airgap radiator?
   */
  auto aerogelZpos      = m_det->constant<double>("DRICH_aerogel_zpos") / dd4hep::mm;
  auto aerogelThickness = m_det->constant<double>("DRICH_aerogel_thickness") / dd4hep::mm;
  auto aerogelMaterial  = m_det->constant<std::string>("DRICH_aerogel_material");
  auto filterZpos       = m_det->constant<double>("DRICH_filter_zpos") / dd4hep::mm;
  auto filterThickness  = m_det->constant<double>("DRICH_filter_thickness") / dd4hep::mm;
  auto filterMaterial   = m_det->constant<std::string>("DRICH_filter_material");
  m_aerogelFlatSurface  = new FlatSurface(TVector3(0, 0, aerogelZpos), normX, normY);
  m_filterFlatSurface   = new FlatSurface(TVector3(0, 0, filterZpos),  normX, normY);
  for (int isec = 0; isec < nSectors; isec++) {
    auto aerogelFlatRadiator = m_irtDetectorCollection->AddFlatRadiator(
        m_irtDetector,                  // Cherenkov detector
        RadiatorName(kAerogel).c_str(), // name
        isec,                           // path
        (G4LogicalVolume*)(0x1),        // G4LogicalVolume (inaccessible? use an integer instead)
        nullptr,                        // G4RadiatorMaterial
        m_aerogelFlatSurface,           // surface
        aerogelThickness                // surface thickness
        );
    auto filterFlatRadiator = m_irtDetectorCollection->AddFlatRadiator(
        m_irtDetector,           // Cherenkov detector
        "Filter",                // name
        isec,                    // path
        (G4LogicalVolume*)(0x2), // G4LogicalVolume (inaccessible? use an integer instead)
        nullptr,                 // G4RadiatorMaterial
        m_filterFlatSurface,     // surface
        filterThickness          // surface thickness
        );
    aerogelFlatRadiator->SetAlternativeMaterialName(aerogelMaterial.c_str());
    filterFlatRadiator->SetAlternativeMaterialName(filterMaterial.c_str());
  }
  m_log->debug("aerogelZpos = {:f} mm", aerogelZpos);
  m_log->debug("filterZpos  = {:f} mm", filterZpos);
  m_log->debug("aerogel thickness = {:f} mm", aerogelThickness);
  m_log->debug("filter thickness  = {:f} mm", filterThickness);

  // sector loop
  for (int isec = 0; isec < nSectors; isec++) {
    std::string secName = "sec" + std::to_string(isec);

    // mirrors
    auto mirrorRadius = m_det->constant<double>("DRICH_mirror_radius") / dd4hep::mm;
    dd4hep::Position mirrorCenter(
      m_det->constant<double>("DRICH_mirror_center_x_"+secName) / dd4hep::mm,
      m_det->constant<double>("DRICH_mirror_center_y_"+secName) / dd4hep::mm,
      m_det->constant<double>("DRICH_mirror_center_z_"+secName) / dd4hep::mm
      );
    m_mirrorSphericalSurface = new SphericalSurface(TVector3(mirrorCenter.x(), mirrorCenter.y(), mirrorCenter.z()), mirrorRadius);
    m_mirrorOpticalBoundary  = new OpticalBoundary(
        m_irtDetector->GetContainerVolume(), // CherenkovRadiator radiator
        m_mirrorSphericalSurface,            // surface
        false                                // bool refractive
        );
    m_irtDetector->AddOpticalBoundary(isec, m_mirrorOpticalBoundary);
    m_log->debug("");
    m_log->debug("  SECTOR {:d} MIRROR:", isec);
    m_log->debug("    mirror x = {:f} mm", mirrorCenter.x());
    m_log->debug("    mirror y = {:f} mm", mirrorCenter.y());
    m_log->debug("    mirror z = {:f} mm", mirrorCenter.z());
    m_log->debug("    mirror R = {:f} mm", mirrorRadius);

    // complete the radiator volume description; this is the rear side of the container gas volume
    m_irtDetector->GetRadiator(RadiatorName(kGas).c_str())->m_Borders[isec].second = m_mirrorSphericalSurface;

    // sensor sphere (only used for validation of sensor normals)
    auto sensorSphRadius  = m_det->constant<double>("DRICH_sensor_sph_radius") / dd4hep::mm;
    auto sensorThickness  = m_det->constant<double>("DRICH_sensor_thickness") / dd4hep::mm;
    auto sensorSize       = m_det->constant<double>("DRICH_sensor_size") / dd4hep::mm;
    dd4hep::Position sensorSphCenter(
      m_det->constant<double>("DRICH_sensor_sph_center_x_"+secName) / dd4hep::mm,
      m_det->constant<double>("DRICH_sensor_sph_center_y_"+secName) / dd4hep::mm,
      m_det->constant<double>("DRICH_sensor_sph_center_z_"+secName) / dd4hep::mm
      );
    m_log->debug("  SECTOR {:d} SENSOR SPHERE:", isec);
    m_log->debug("    sphere x = {:f} mm", sensorSphCenter.x());
    m_log->debug("    sphere y = {:f} mm", sensorSphCenter.y());
    m_log->debug("    sphere z = {:f} mm", sensorSphCenter.z());
    m_log->debug("    sphere R = {:f} mm", sensorSphRadius);

    // sensor modules: search the detector tree for sensors for this sector
    for(auto const& [de_name, detSensor] : m_detRich.children()) {
      if(de_name.find("sensor_de_"+secName)!=std::string::npos) {

        // get sensor info
        auto imodsec = detSensor.id();
        // - get sensor centroid position
        auto pvSensor  = detSensor.placement();
        auto posSensor = (1/dd4hep::mm) * (m_posRich + pvSensor.position());
        // - get sensor surface position
        dd4hep::Direction radialDir = posSensor - sensorSphCenter; // sensor sphere radius direction
        auto surfaceOffset = radialDir.Unit() * (0.5*sensorThickness);
        auto posSensorSurface = posSensor + surfaceOffset;
        // - add to `m_sensor_info` map
        richgeo::Sensor sensor_info;
        sensor_info.size             = sensorSize;
        sensor_info.surface_centroid = posSensorSurface;
        sensor_info.surface_offset   = surfaceOffset;
        m_sensor_info.insert({ imodsec, sensor_info });
        // - get surface normal and in-plane vectors
        double sensorLocalNormX[3] = {1.0, 0.0, 0.0};
        double sensorLocalNormY[3] = {0.0, 1.0, 0.0};
        double sensorGlobalNormX[3], sensorGlobalNormY[3];
        pvSensor.ptr()->LocalToMasterVect(sensorLocalNormX, sensorGlobalNormX); // ignore vessel transformation, since it is a pure translation
        pvSensor.ptr()->LocalToMasterVect(sensorLocalNormY, sensorGlobalNormY);

        // validate sensor position and normal
        // - test normal vectors
        dd4hep::Direction normXdir, normYdir;
        normXdir.SetCoordinates(sensorGlobalNormX);
        normYdir.SetCoordinates(sensorGlobalNormY);
        auto normZdir   = normXdir.Cross(normYdir);         // sensor surface normal
        auto testOrtho  = normXdir.Dot(normYdir);           // should be zero, if normX and normY are orthogonal
        auto testRadial = radialDir.Cross(normZdir).Mag2(); // should be zero, if sensor surface normal is parallel to sensor sphere radius
        if(abs(testOrtho)>1e-6 || abs(testRadial)>1e-6) {
          m_log->error(
              "sensor normal is wrong: normX.normY = {:f}   |radialDir x normZdir|^2 = {:f}",
              testOrtho,
              testRadial
              );
          return;
        }
        // - test sensor positioning
        auto distSensor2center = sqrt((posSensorSurface-sensorSphCenter).Mag2()); // distance between sensor sphere center and sensor surface position
        auto testDist          = abs(distSensor2center-sensorSphRadius);          // should be zero, if sensor position w.r.t. sensor sphere center is correct
        if(abs(testDist)>1e-5) {
          m_log->error(
              "sensor positioning is wrong: dist(sensor, sphere_center) = {:f},  sphere_radius = {:f},  sensor_thickness = {:f},  |diff| = {:g}\n",
              distSensor2center,
              sensorSphRadius,
              sensorThickness,
              testDist
              );
          return;
        }


        // create the optical surface
        m_sensorFlatSurface = new FlatSurface(
            TVector3(posSensorSurface.x(), posSensorSurface.y(), posSensorSurface.z()),
            TVector3(sensorGlobalNormX),
            TVector3(sensorGlobalNormY)
            );
        m_irtDetector->CreatePhotonDetectorInstance(
            isec,                // sector
            m_irtPhotonDetector, // CherenkovPhotonDetector
            imodsec,             // copy number
            m_sensorFlatSurface  // surface
            );
        m_log->trace(
            "sensor: id={:#X} pos=({:5.2f}, {:5.2f}, {:5.2f}) normX=({:5.2f}, {:5.2f}, {:5.2f}) normY=({:5.2f}, {:5.2f}, {:5.2f})",
            imodsec,
            posSensorSurface.x(), posSensorSurface.y(), posSensorSurface.z(),
            normXdir.x(),  normXdir.y(),  normXdir.z(),
            normYdir.x(),  normYdir.y(),  normYdir.z()
            );
      }
    } // search for sensors

  } // sector loop

  // set reference refractive indices // NOTE: numbers may be overridden externally
  std::map<const char*, double> rIndices;
  rIndices.insert({RadiatorName(kGas).c_str(),     1.00076});
  rIndices.insert({RadiatorName(kAerogel).c_str(), 1.0190});
  rIndices.insert({"Filter",                       1.5017});
  for (auto const& [rName, rIndex] : rIndices) {
    auto rad = m_irtDetector->GetRadiator(rName);
    if (rad)
      rad->SetReferenceRefractiveIndex(rIndex);
  }

  // set refractive index table
  SetRefractiveIndexTable();

  // define the `cell ID -> pixel position` converter
  SetReadoutIDToPositionLambda();
}

// destructor
richgeo::IrtGeoDRICH::~IrtGeoDRICH() {
  delete m_surfEntrance;
  delete m_irtPhotonDetector;
  delete m_aerogelFlatSurface;
  delete m_filterFlatSurface;
  delete m_mirrorSphericalSurface;
  delete m_mirrorOpticalBoundary;
  delete m_sensorFlatSurface;
}

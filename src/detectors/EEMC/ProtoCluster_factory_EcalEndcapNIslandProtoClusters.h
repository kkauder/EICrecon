// Copyright 2022, David Lawrence
// Subject to the terms in the LICENSE file found in the top-level directory.
//

#pragma once

#include <random>

#include "extensions/jana/JChainFactoryT.h"
#include "services/geometry/dd4hep/JDD4hep_service.h"
#include "algorithms/calorimetry/CalorimeterIslandCluster.h"
#include "services/log/Log_service.h"
#include "extensions/spdlog/SpdlogExtensions.h"

class ProtoCluster_factory_EcalEndcapNIslandProtoClusters : public JChainFactoryT<edm4eic::ProtoCluster>, CalorimeterIslandCluster {

public:
    //------------------------------------------
    // Constructor
    ProtoCluster_factory_EcalEndcapNIslandProtoClusters(std::vector<std::string> default_input_tags)
    : JChainFactoryT<edm4eic::ProtoCluster>(std::move(default_input_tags)) {
        m_log = japp->GetService<Log_service>()->logger(GetTag());
    }

    //------------------------------------------
    // Init
    void Init() override{
        InitDataTags(GetPluginName() + ":" + GetTag());

        auto app = GetApplication();

        // adjacency matrix
        m_geoSvcName = "GeoSvc";
        u_adjacencyMatrix = "(abs(row_1 - row_2) + abs(column_1 - column_2)) == 1";
        u_adjacencyMatrix.erase(
          std::remove_if(u_adjacencyMatrix.begin(), u_adjacencyMatrix.end(), ::isspace),
          u_adjacencyMatrix.end());
        m_readout = "EcalEndcapNHits";

        // neighbour checking distances
        m_sectorDist=5.0 * dd4hep::cm;             // from ATHENA reconstruction.py
        u_localDistXY={};     //{this, "localDistXY", {}};
        u_localDistXZ={};     //{this, "localDistXZ", {}};
        u_localDistYZ={};     //{this, "localDistYZ", {}};
        u_globalDistRPhi={};  //{this, "globalDistRPhi", {}};
        u_globalDistEtaPhi={};//{this, "globalDistEtaPhi", {}};
        u_dimScaledLocalDistXY={};

        m_splitCluster = true;
        m_minClusterHitEdep=1.0 * dd4hep::MeV;
        m_minClusterCenterEdep=30.0 * dd4hep::MeV;
        u_transverseEnergyProfileMetric = "globalDistEtaPhi";
        u_transverseEnergyProfileScale = 0.08;

        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:geoServiceName", m_geoSvcName);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:readoutClass", m_readout);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:sectorDist",   m_sectorDist);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:localDistXY",   u_localDistXY);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:localDistXZ",   u_localDistXZ);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:localDistYZ",  u_localDistYZ);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:globalDistRPhi",    u_globalDistRPhi);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:globalDistEtaPhi",    u_globalDistEtaPhi);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:dimScaledLocalDistXY",    u_dimScaledLocalDistXY);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:adjacencyMatrix", u_adjacencyMatrix);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:splitCluster", m_splitCluster);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:minClusterHitEdep", m_minClusterHitEdep);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:minClusterCenterEdep", m_minClusterCenterEdep);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:transverseEnergyProfileMetric", u_transverseEnergyProfileMetric);
        app->SetDefaultParameter("EEMC:EcalEndcapNIslandProtoClusters:transverseEnergyProfileScale", u_transverseEnergyProfileScale);
        m_geoSvc = app->template GetService<JDD4hep_service>();

        AlgorithmInit(m_log);
    }

    //------------------------------------------
    // ChangeRun
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override{
        AlgorithmChangeRun();
    }

    //------------------------------------------
    // Process
    void Process(const std::shared_ptr<const JEvent> &event) override{
        // Prefill inputs
        hits = event->Get<edm4eic::CalorimeterHit>(GetInputTags()[0]);

        // Call Process for generic algorithm
        AlgorithmProcess();

        // Hand owner of algorithm objects over to JANA
        Set(protoClusters);
        protoClusters.clear(); // not really needed, but better to not leave dangling pointers around
    }
};

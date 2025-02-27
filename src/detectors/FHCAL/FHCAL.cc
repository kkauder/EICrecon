// Copyright 2022, David Lawrence
// Subject to the terms in the LICENSE file found in the top-level directory.
//
//

#include "extensions/jana/JChainFactoryGeneratorT.h"
#include "extensions/jana/JChainMultifactoryGeneratorT.h"

#include "factories/calorimetry/CalorimeterClusterRecoCoG_factoryT.h"
#include "factories/calorimetry/CalorimeterHitDigi_factoryT.h"
#include "factories/calorimetry/CalorimeterHitReco_factoryT.h"
#include "factories/calorimetry/CalorimeterHitsMerger_factoryT.h"
#include "factories/calorimetry/CalorimeterTruthClustering_factoryT.h"

#include "ProtoCluster_factory_HcalEndcapPIslandProtoClusters.h"

#include "ProtoCluster_factory_HcalEndcapPInsertIslandProtoClusters.h"

#include "ProtoCluster_factory_LFHCALIslandProtoClusters.h"

extern "C" {
    void InitPlugin(JApplication *app) {

        using namespace eicrecon;

        InitJANAPlugin(app);

        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitDigi_factoryT>(
           "HcalEndcapPRawHits", {"HcalEndcapPHits"}, {"HcalEndcapPRawHits"},
           {
             .eRes = {},
             .tRes = 0.001 * dd4hep::ns,
             .capADC = 65536,
             .dyRangeADC = 1 * dd4hep::GeV,
             .pedMeanADC = 20,
             .pedSigmaADC = 0.8,
             .resolutionTDC = 10 * dd4hep::picosecond,
             .corrMeanScale = 1.0,
           },
           app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitReco_factoryT>(
          "HcalEndcapPRecHits", {"HcalEndcapPRawHits"}, {"HcalEndcapPRecHits"},
          {
            .capADC = 65536,
            .dyRangeADC = 1 * dd4hep::GeV,
            .pedMeanADC = 20,
            .pedSigmaADC = 0.8,
            .resolutionTDC = 10 * dd4hep::picosecond,
            .thresholdFactor = 1.0,
            .thresholdValue = 3.0,
            .sampFrac = 0.033,
            .readout = "HcalEndcapPHits",
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitsMerger_factoryT>(
          "HcalEndcapPMergedHits", {"HcalEndcapPRecHits"}, {"HcalEndcapPMergedHits"},
          {
            .readout = "HcalEndcapPHits",
            .fields = {"layer", "slice"},
            .refs = {1, 0},
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterTruthClustering_factoryT>(
          "HcalEndcapPTruthProtoClusters", {"HcalEndcapPRecHits", "HcalEndcapPHits"}, {"HcalEndcapPTruthProtoClusters"},
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainFactoryGeneratorT<ProtoCluster_factory_HcalEndcapPIslandProtoClusters>(
          {"HcalEndcapPRecHits"}, "HcalEndcapPIslandProtoClusters"
        ));

        app->Add(
          new JChainMultifactoryGeneratorT<CalorimeterClusterRecoCoG_factoryT>(
             "HcalEndcapPTruthClusters",
            {"HcalEndcapPTruthProtoClusters",        // edm4eic::ProtoClusterCollection
             "HcalEndcapPHits"},                     // edm4hep::SimCalorimeterHitCollection
            {"HcalEndcapPTruthClusters",             // edm4eic::Cluster
             "HcalEndcapPTruthClusterAssociations"}, // edm4eic::MCRecoClusterParticleAssociation
            {
              .energyWeight = "log",
              .moduleDimZName = "",
              .sampFrac = 1.0,
              .logWeightBase = 3.6,
              .depthCorrection = 0.0,
              .enableEtaBounds = false
            },
            app   // TODO: Remove me once fixed
          )
        );

        app->Add(
          new JChainMultifactoryGeneratorT<CalorimeterClusterRecoCoG_factoryT>(
             "HcalEndcapPClusters",
            {"HcalEndcapPIslandProtoClusters",  // edm4eic::ProtoClusterCollection
             "HcalEndcapPHits"},                // edm4hep::SimCalorimeterHitCollection
            {"HcalEndcapPClusters",             // edm4eic::Cluster
             "HcalEndcapPClusterAssociations"}, // edm4eic::MCRecoClusterParticleAssociation
            {
              .energyWeight = "log",
              .moduleDimZName = "",
              .sampFrac = 0.033,
              .logWeightBase = 6.2,
              .depthCorrection = 0.0,
              .enableEtaBounds = false,
            },
            app   // TODO: Remove me once fixed
          )
        );

        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitDigi_factoryT>(
           "HcalEndcapPInsertRawHits", {"HcalEndcapPInsertHits"}, {"HcalEndcapPInsertRawHits"},
           {
             .eRes = {},
             .tRes = 0.0 * dd4hep::ns,
             .capADC = 32768,
             .dyRangeADC = 200 * dd4hep::MeV,
             .pedMeanADC = 400,
             .pedSigmaADC = 10,
             .resolutionTDC = 10 * dd4hep::picosecond,
             .corrMeanScale = 1.0,
           },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitReco_factoryT>(
          "HcalEndcapPInsertRecHits", {"HcalEndcapPInsertRawHits"}, {"HcalEndcapPInsertRecHits"},
          {
            .capADC = 32768,
            .dyRangeADC = 200. * dd4hep::MeV,
            .pedMeanADC = 400,
            .pedSigmaADC = 10.,
            .resolutionTDC = 10 * dd4hep::picosecond,
            .thresholdFactor = 0.,
            .thresholdValue = -100.,
            .sampFrac = 0.0098,
            .readout = "HcalEndcapPInsertHits",
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitsMerger_factoryT>(
          "HcalEndcapPInsertMergedHits", {"HcalEndcapPInsertRecHits"}, {"HcalEndcapPInsertMergedHits"},
          {
            .readout = "HcalEndcapPInsertHits",
            .fields = {"layer", "slice"},
            .refs = {1, 0},
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterTruthClustering_factoryT>(
          "HcalEndcapPInsertTruthProtoClusters", {"HcalEndcapPInsertMergedHits", "HcalEndcapPInsertHits"}, {"HcalEndcapPInsertTruthProtoClusters"},
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainFactoryGeneratorT<ProtoCluster_factory_HcalEndcapPInsertIslandProtoClusters>(
          {"HcalEndcapPInsertMergedHits"}, "HcalEndcapPInsertIslandProtoClusters"
        ));

        app->Add(
          new JChainMultifactoryGeneratorT<CalorimeterClusterRecoCoG_factoryT>(
             "HcalEndcapPInsertTruthClusters",
            {"HcalEndcapPInsertTruthProtoClusters",        // edm4eic::ProtoClusterCollection
             "HcalEndcapPInsertHits"},                     // edm4hep::SimCalorimeterHitCollection
            {"HcalEndcapPInsertTruthClusters",             // edm4eic::Cluster
             "HcalEndcapPInsertTruthClusterAssociations"}, // edm4eic::MCRecoClusterParticleAssociation
            {
              .energyWeight = "log",
              .moduleDimZName = "",
              .sampFrac = 1.0,
              .logWeightBase = 3.6,
              .depthCorrection = 0.0,
              .enableEtaBounds = true
            },
            app   // TODO: Remove me once fixed
          )
        );

        app->Add(
          new JChainMultifactoryGeneratorT<CalorimeterClusterRecoCoG_factoryT>(
             "HcalEndcapPInsertClusters",
            {"HcalEndcapPInsertIslandProtoClusters",  // edm4eic::ProtoClusterCollection
             "HcalEndcapPInsertHits"},                // edm4hep::SimCalorimeterHitCollection
            {"HcalEndcapPInsertClusters",             // edm4eic::Cluster
             "HcalEndcapPInsertClusterAssociations"}, // edm4eic::MCRecoClusterParticleAssociation
            {
              .energyWeight = "log",
              .moduleDimZName = "",
              .sampFrac = 1.0,
              .logWeightBase = 6.2,
              .depthCorrection = 0.0,
              .enableEtaBounds = false,
            },
            app   // TODO: Remove me once fixed
          )
        );

        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitDigi_factoryT>(
          "LFHCALRawHits", {"LFHCALHits"}, {"LFHCALRawHits"},
          {
            .eRes = {},
            .tRes = 0.0 * dd4hep::ns,
            .capADC = 65536,
            .capTime = 100,
            .dyRangeADC = 1 * dd4hep::GeV,
            .pedMeanADC = 20,
            .pedSigmaADC = 0.8,
            .resolutionTDC = 10 * dd4hep::picosecond,
            .corrMeanScale = 1.0,
            .readout = "LFHCALHits",
            .fields = {"layerz"},
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitReco_factoryT>(
          "LFHCALRecHits", {"LFHCALRawHits"}, {"LFHCALRecHits"},
          {
            .capADC = 65536,
            .dyRangeADC = 1 * dd4hep::GeV,
            .pedMeanADC = 20,
            .pedSigmaADC = 0.8,
            .resolutionTDC = 10 * dd4hep::picosecond,
            .thresholdFactor = 1.0,
            .thresholdValue = 3.0,
            .sampFrac = 0.033,
            .sampFracLayer = {
              0.019, //  0
              0.037, //  1
              0.037, //  2
              0.037, //  3
              0.037, //  4
              0.037, //  5
              0.037, //  6
              0.037, //  7
              0.037, //  8
              0.037, //  9
              0.037, // 10
              0.037, // 11
              0.037, // 12
              0.037, // 13
            },
            .readout = "LFHCALHits",
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterTruthClustering_factoryT>(
          "LFHCALTruthProtoClusters", {"LFHCALRecHits", "LFHCALHits"}, {"LFHCALTruthProtoClusters"},
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainFactoryGeneratorT<ProtoCluster_factory_LFHCALIslandProtoClusters>(
          {"LFHCALRecHits"}, "LFHCALIslandProtoClusters"
        ));

        app->Add(
          new JChainMultifactoryGeneratorT<CalorimeterClusterRecoCoG_factoryT>(
             "LFHCALTruthClusters",
            {"LFHCALTruthProtoClusters",        // edm4eic::ProtoClusterCollection
             "LFHCALHits"},                     // edm4hep::SimCalorimeterHitCollection
            {"LFHCALTruthClusters",             // edm4eic::Cluster
             "LFHCALTruthClusterAssociations"}, // edm4eic::MCRecoClusterParticleAssociation
            {
              .energyWeight = "log",
              .moduleDimZName = "",
              .sampFrac = 1.0,
              .logWeightBase = 4.5,
              .depthCorrection = 0.0,
              .enableEtaBounds = false
            },
            app   // TODO: Remove me once fixed
          )
        );

        app->Add(
          new JChainMultifactoryGeneratorT<CalorimeterClusterRecoCoG_factoryT>(
             "LFHCALClusters",
            {"LFHCALIslandProtoClusters",  // edm4eic::ProtoClusterCollection
             "LFHCALHits"},                // edm4hep::SimCalorimeterHitCollection
            {"LFHCALClusters",             // edm4eic::Cluster
             "LFHCALClusterAssociations"}, // edm4eic::MCRecoClusterParticleAssociation
            {
              .energyWeight = "log",
              .moduleDimZName = "",
              .sampFrac = 1.0,
              .logWeightBase = 4.5,
              .depthCorrection = 0.0,
              .enableEtaBounds = false,
            },
            app   // TODO: Remove me once fixed
          )
        );
    }
}

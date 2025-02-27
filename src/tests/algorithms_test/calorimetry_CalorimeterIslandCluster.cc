// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2023, Dmitry Kalinkin

#include <limits>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <spdlog/logger.h>

#include "algorithms/calorimetry/CalorimeterIslandCluster.h"


TEST_CASE( "the clustering algorithm runs", "[CalorimeterIslandCluster]" ) {
  CalorimeterIslandCluster algo;

  std::shared_ptr<spdlog::logger> logger = spdlog::default_logger()->clone("CalorimeterIslandCluster");
  logger->set_level(spdlog::level::trace);

  algo.m_minClusterHitEdep = 0. * dd4hep::GeV;
  algo.m_minClusterCenterEdep = 0. * dd4hep::GeV;

  SECTION( "without splitting" ) {
    algo.m_splitCluster = false;
    algo.u_localDistXY = {1 * dd4hep::mm, 1 * dd4hep::mm};
    algo.AlgorithmInit(logger);
    algo.AlgorithmChangeRun();

    SECTION( "on a single cell" ) {
      algo.hits = {
        new edm4eic::CalorimeterHit(
          0, // std::uint64_t cellID,
          5.0, // float energy,
          0.0, // float energyError,
          0.0, // float time,
          0.0, // float timeError,
          {0.0, 0.0, 0.0}, // edm4hep::Vector3f position,
          {0.0, 0.0, 0.0}, // edm4hep::Vector3f dimension,
          0, // std::int32_t sector,
          0, // std::int32_t layer,
          {0.0, 0.0, 0.0} // edm4hep::Vector3f local
        )
      };
      algo.AlgorithmProcess();

      REQUIRE( algo.protoClusters.size() == 1 );
      REQUIRE( algo.protoClusters[0]->hits_size() == 1 );
      REQUIRE( algo.protoClusters[0]->weights_size() == 1 );
    }

    SECTION( "on two separated cells" ) {
      algo.hits = {
        new edm4eic::CalorimeterHit(
          0, // std::uint64_t cellID,
          5.0, // float energy,
          0.0, // float energyError,
          0.0, // float time,
          0.0, // float timeError,
          {0.0, 0.0, 0.0}, // edm4hep::Vector3f position,
          {1.0, 1.0, 0.0}, // edm4hep::Vector3f dimension,
          0, // std::int32_t sector,
          0, // std::int32_t layer,
          {0.0, 0.0, 0.0} // edm4hep::Vector3f local
        ),
        new edm4eic::CalorimeterHit(
          1, // std::uint64_t cellID,
          6.0, // float energy,
          0.0, // float energyError,
          0.0, // float time,
          0.0, // float timeError,
          {0.0, 0.0, 0.0}, // edm4hep::Vector3f position,
          {1.0, 1.0, 0.0}, // edm4hep::Vector3f dimension,
          0, // std::int32_t sector,
          0, // std::int32_t layer,
          {1.1 /* mm */, 1.1 /* mm */, 0.0} // edm4hep::Vector3f local
        )
      };
      algo.AlgorithmProcess();

      REQUIRE( algo.protoClusters.size() == 2 );
      REQUIRE( algo.protoClusters[0]->hits_size() == 1 );
      REQUIRE( algo.protoClusters[0]->weights_size() == 1 );
      REQUIRE( algo.protoClusters[1]->hits_size() == 1 );
      REQUIRE( algo.protoClusters[1]->weights_size() == 1 );
    }

    SECTION( "on two adjacent cells" ) {
      algo.hits = {
        new edm4eic::CalorimeterHit(
          0, // std::uint64_t cellID,
          5.0, // float energy,
          0.0, // float energyError,
          0.0, // float time,
          0.0, // float timeError,
          {0.0, 0.0, 0.0}, // edm4hep::Vector3f position,
          {1.0, 1.0, 0.0}, // edm4hep::Vector3f dimension,
          0, // std::int32_t sector,
          0, // std::int32_t layer,
          {0.0, 0.0, 0.0} // edm4hep::Vector3f local
        ),
        new edm4eic::CalorimeterHit(
          1, // std::uint64_t cellID,
          6.0, // float energy,
          0.0, // float energyError,
          0.0, // float time,
          0.0, // float timeError,
          {0.0, 0.0, 0.0}, // edm4hep::Vector3f position,
          {1.0, 1.0, 0.0}, // edm4hep::Vector3f dimension,
          0, // std::int32_t sector,
          0, // std::int32_t layer,
          {0.9 /* mm */, 0.9 /* mm */, 0.0} // edm4hep::Vector3f local
        )
      };
      algo.AlgorithmProcess();

      REQUIRE( algo.protoClusters.size() == 1 );
      REQUIRE( algo.protoClusters[0]->hits_size() == 2 );
      REQUIRE( algo.protoClusters[0]->weights_size() == 2 );
    }
  }

  SECTION( "run on three adjacent cells" ) {
    SECTION( "with splitting" ) {
      algo.m_splitCluster = true;
      algo.u_transverseEnergyProfileMetric = "localDistXY";
      algo.u_transverseEnergyProfileScale = std::numeric_limits<decltype(algo.u_transverseEnergyProfileScale)>::infinity();
    }
    SECTION( "without splitting" ) {
      algo.m_splitCluster = false;
    }
    algo.u_localDistXY = {1 * dd4hep::mm, 1 * dd4hep::mm};
    algo.AlgorithmInit(logger);
    algo.AlgorithmChangeRun();

    algo.hits = {
      new edm4eic::CalorimeterHit(
        0, // std::uint64_t cellID,
        5.0, // float energy,
        0.0, // float energyError,
        0.0, // float time,
        0.0, // float timeError,
        {0.0, 0.0, 0.0}, // edm4hep::Vector3f position,
        {1.0, 1.0, 0.0}, // edm4hep::Vector3f dimension,
        0, // std::int32_t sector,
        0, // std::int32_t layer,
        {0.0, 0.0, 0.0} // edm4hep::Vector3f local
      ),
      new edm4eic::CalorimeterHit(
        1, // std::uint64_t cellID,
        1.0, // float energy,
        0.0, // float energyError,
        0.0, // float time,
        0.0, // float timeError,
        {0.0, 0.0, 0.0}, // edm4hep::Vector3f position,
        {1.0, 1.0, 0.0}, // edm4hep::Vector3f dimension,
        0, // std::int32_t sector,
        0, // std::int32_t layer,
        {0.9 /* mm */, 0.9 /* mm */, 0.0} // edm4hep::Vector3f local
      ),
      new edm4eic::CalorimeterHit(
        1, // std::uint64_t cellID,
        6.0, // float energy,
        0.0, // float energyError,
        0.0, // float time,
        0.0, // float timeError,
        {0.0, 0.0, 0.0}, // edm4hep::Vector3f position,
        {1.0, 1.0, 0.0}, // edm4hep::Vector3f dimension,
        0, // std::int32_t sector,
        0, // std::int32_t layer,
        {1.8 /* mm */, 1.8 /* mm */, 0.0} // edm4hep::Vector3f local
      )
    };
    algo.AlgorithmProcess();

    if (algo.m_splitCluster) {
      REQUIRE( algo.protoClusters.size() == 2 );
      REQUIRE( algo.protoClusters[0]->hits_size() == 3 );
      REQUIRE( algo.protoClusters[0]->weights_size() == 3 );
      for (double weight : algo.protoClusters[0]->getWeights()) {
        double energy_fraction = algo.hits[0]->getEnergy() / (algo.hits[0]->getEnergy() + algo.hits[2]->getEnergy());
        REQUIRE_THAT( weight, Catch::Matchers::WithinAbs(energy_fraction, 1e-5) );
      }
      REQUIRE( algo.protoClusters[1]->hits_size() == 3 );
      REQUIRE( algo.protoClusters[1]->weights_size() == 3 );
      for (double weight : algo.protoClusters[1]->getWeights()) {
        double energy_fraction = algo.hits[2]->getEnergy() / (algo.hits[0]->getEnergy() + algo.hits[2]->getEnergy());
        REQUIRE_THAT( weight, Catch::Matchers::WithinAbs(energy_fraction, 1e-5) );
      }
    } else {
      REQUIRE( algo.protoClusters.size() == 1 );
      REQUIRE( algo.protoClusters[0]->hits_size() == 3 );
      REQUIRE( algo.protoClusters[0]->weights_size() == 3 );
    }
  }
}

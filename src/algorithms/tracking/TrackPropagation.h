// Created by Dmitry Romanov
// Subject to the terms in the LICENSE file found in the top-level directory.
//

#pragma once

#include <memory>
#include <functional>
#include <spdlog/logger.h>

#include <Acts/Geometry/TrackingGeometry.hpp>
#include <Acts/EventData/TrackParameters.hpp>
#include <Acts/EventData/MultiTrajectory.hpp>
#include <Acts/EventData/MultiTrajectoryHelpers.hpp>

#include "algorithms/tracking/JugTrack/TrackingResultTrajectory.hpp"

#include <edm4eic/TrackSegmentCollection.h>


#include "ActsGeometryProvider.h"
#include "TrackProjector.h"



namespace eicrecon {

    using ActsTrackPropagationResult = Acts::Result<std::unique_ptr<const Acts::BoundTrackParameters>>;

    /** Extract the particles form fit trajectories.
     *
     * \ingroup tracking
     */
    class TrackPropagation {


    public:

        /** Initialize algorithm */
        void init(std::shared_ptr<const ActsGeometryProvider> geo_svc, std::shared_ptr<spdlog::logger> logger);

        /** Propagates a single trajectory to a given surface */
        std::unique_ptr<edm4eic::TrackPoint> propagate(const eicrecon::TrackingResultTrajectory *, const std::shared_ptr<const Acts::Surface>& targetSurf);

        /** Propagates a collection of trajectories to a given surface
         * @remark: being a simple wrapper of propagate(...) this method is more suitable for factories */
        std::vector<std::unique_ptr<edm4eic::TrackPoint>> propagateMany(std::vector<const eicrecon::TrackingResultTrajectory *> trajectories,
                                                         const std::shared_ptr<const Acts::Surface> &targetSurf);

        /** Propagates a collection of trajectories to a list of surfaces, and returns the full `TrackSegment`;
         *  optionally omit track points with `trackPointCut`.
         * @remark: being a simple wrapper of propagate(...) this method is more suitable for factories */
        std::unique_ptr<edm4eic::TrackSegmentCollection> propagateToSurfaceList(
            std::vector<const eicrecon::TrackingResultTrajectory*> trajectories,
            std::vector<std::shared_ptr<Acts::Surface>> targetSurfaces,
            std::function<bool(edm4eic::TrackPoint)> trackPointCut = [] (edm4eic::TrackPoint p) { return true; },
            bool stopIfTrackPointCutFailed = false
            );

    private:

        Acts::GeometryContext m_geoContext;
        Acts::MagneticFieldContext m_fieldContext;
        std::shared_ptr<const ActsGeometryProvider> m_geoSvc;
        std::shared_ptr<spdlog::logger> m_log;
    };
} // namespace eicrecon

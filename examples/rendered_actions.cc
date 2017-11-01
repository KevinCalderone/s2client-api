#include "sc2api/sc2_api.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2renderer/sc2_renderer.h"

#include <iostream>

#define USE_SOFTWARE_RENDERING 0

const char* kReplayFolder = "E:/Replays/";
const int kMapX = 800;
const int kMapY = 600;
const int kMiniMapX = 300;
const int kMiniMapY = 300;

const sc2::Point2DI kMapOrigin(kMiniMapX, 0);
const sc2::Point2DI kMinimapOrigin(0, kMapY - kMiniMapY);
static_assert(kMapX > kMiniMapX && kMapY > kMiniMapY, "");

struct SRect {
    int x;
    int y;
    int w;
    int h;
};

static SRect MapDot(const sc2::Point2DI& coord, int size = 10) {
    return {
        coord.x - size + kMapOrigin.x,
        coord.y - size + kMapOrigin.y,
        size * 2,
        size * 2
    };
}

static SRect MinimapDot(const sc2::Point2DI& coord, int size = 10) {
    return {
        coord.x - size + kMinimapOrigin.x,
        coord.y - size + kMinimapOrigin.y,
        size * 2,
        size * 2
    };
}

static SRect MapRect(const sc2::Point2DI& start, const sc2::Point2DI& end) {
    return {
        start.x + kMapOrigin.x, 
        start.y + kMapOrigin.y, 
        end.x - start.x, 
        end.y - start.y
    };
}

class Replay : public sc2::ReplayObserver {
public:
    void OnGameStart() final {
        sc2::renderer::Initialize("Rendered", 50, 50, kMiniMapX + kMapX, kMapY);
    }

    void ProcessActions() {
        const auto& actions = Observation()->GetRenderedActions();
        for (const auto& action : actions.unit_commands) {
            if (action.target_type == sc2::SpatialUnitCommand::TargetScreen) {
                last_action_visual = MapDot(action.target);
                return;
            }
            else if (action.target_type == sc2::SpatialUnitCommand::TargetMinimap) {
                last_action_visual = MinimapDot(action.target);
                return;
            }
        }
        for (const auto& action : actions.select_points) {
            last_action_visual = MapDot(action.select_screen);
            return;
        }
        for (const auto& action : actions.select_rects) {
            for (const auto& rect : action.select_screen) {
                last_action_visual = MapRect(rect.from, rect.to);
                return;
            }
        }
        for (const auto& action : actions.camera_moves) {
            last_action_visual = MinimapDot(action.center_minimap);
            return;
        }
    }

    void RenderGame() {
        const SC2APIProtocol::Observation* observation = Observation()->GetRawObservation();
        const SC2APIProtocol::ObservationRender& render = observation->render_data();

        const SC2APIProtocol::ImageData& map = render.map();
        sc2::renderer::ImageRGB(&map.data().data()[0], map.size().x(), map.size().y(), kMapOrigin.x, kMapOrigin.y);

        const SC2APIProtocol::ImageData& minimap = render.minimap();
        sc2::renderer::ImageRGB(&minimap.data().data()[0], minimap.size().x(), minimap.size().y(), kMinimapOrigin.x, kMinimapOrigin.y);
    }

    void RenderActions() {
        sc2::renderer::Rect(
            last_action_visual.x,
            last_action_visual.y,
            last_action_visual.w,
            last_action_visual.h,
            255,
            0,
            0
        );
    }

    void OnStep() final {
        ProcessActions();
        RenderGame();
        RenderActions();
        sc2::renderer::Render();
    }

    void OnGameEnd() final {
        sc2::renderer::Shutdown();
    }

private:
    SRect last_action_visual = {0,0,0,0};
};

int main(int argc, char* argv[]) {
    sc2::Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv)) {
        std::cout << "Unable to find or parse settings." << std::endl;
        return 1;
    }

    if (!coordinator.SetReplayPath(kReplayFolder)) {
        std::cout << "Unable to find replays." << std::endl;
        return 1;
    }

    sc2::RenderSettings settings(kMapX, kMapY, kMiniMapX, kMiniMapY);
    coordinator.SetRender(settings);

#if defined(__linux__)
#if USE_SOFTWARE_RENDERING
    coordinator.AddCommandLine("-eglpath /usr/lib/nvidia-367/libEGL.so");
#else
    coordinator.AddCommandLine("-osmesapath libOSMesa.so");
#endif
#endif

    Replay replay_observer;
    coordinator.AddReplayObserver(&replay_observer);

    while (coordinator.Update()) {
        sc2::SleepFor(10);
    }
    while (!sc2::PollKeyPress());

    return 0;
}

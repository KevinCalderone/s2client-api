#include "sc2api/sc2_api.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2renderer/sc2_renderer.h"

#include <iostream>
#include <vector>

const int kMapX = 800;
const int kMapY = 600;
const int kMiniMapX = 300;
const int kMiniMapY = 300;

const char* kReplayFolder = "E:/Replays/";

struct SRect {
    int x;
    int y;
    int w;
    int h;
    int r;
    int g;
    int b;
};
SRect g_rect;

class Replay : public sc2::ReplayObserver {
public:
    void OnGameStart() final {
        sc2::renderer::Initialize("Rendered", 50, 50, kMiniMapX + kMapX, std::max(kMiniMapY, kMapY));
    }

    void DrawDotMap(const sc2::Point2DI& coord) {
        g_rect = {
            coord.x - 10 + kMiniMapX, coord.y - 10, 20, 20, 255, 0, 0
        };
    }

    void DrawRectMap(const sc2::Point2DI& start, const sc2::Point2DI& end) {
        g_rect = {
            start.x + kMiniMapX, start.y, end.x - start.x, end.y - start.y, 255, 0, 0        
        };
    }

    void DrawDotMinimap(const sc2::Point2DI& coord) {
        g_rect = {
            coord.x - 10, coord.y - 10 + (kMapY - kMiniMapY), 20, 20, 255, 0, 0
        };
    }

    void DrawActions() {
        const auto& actions = Observation()->GetRenderedActions();
        for (const auto& action : actions.unit_commands) {
            if (action.target_type == sc2::SpatialUnitCommand::TargetScreen) {
                DrawDotMap(action.target);
                return;
            }
            else if (action.target_type == sc2::SpatialUnitCommand::TargetMinimap) {
                DrawDotMinimap(action.target);
                return;
            }
        }
        for (const auto& action : actions.select_points) {
            DrawDotMap(action.select_screen);
            return;
        }
        for (const auto& action : actions.camera_moves) {
            DrawDotMinimap(action.center_minimap);
            return;
        }
        for (const auto& action : actions.select_rects) {
            for (const auto& rect : action.select_screen) {
                DrawRectMap(rect.from, rect.to);
                return;
            }
        }
    }

    void ActuallyDrawActions() {
        sc2::renderer::Rect(
            g_rect.x,
            g_rect.y,
            g_rect.w,
            g_rect.h,
            g_rect.r,
            g_rect.g,
            g_rect.b
        );
    }

    void OnStep() final {
        const SC2APIProtocol::Observation* observation = Observation()->GetRawObservation();
        const SC2APIProtocol::ObservationRender& render = observation->render_data();

        const SC2APIProtocol::ImageData& minimap = render.minimap();
        sc2::renderer::ImageRGB(&minimap.data().data()[0], minimap.size().x(), minimap.size().y(), 0, std::max(kMiniMapY, kMapY) - kMiniMapY);

        const SC2APIProtocol::ImageData& map = render.map();
        sc2::renderer::ImageRGB(&map.data().data()[0], map.size().x(), map.size().y(), kMiniMapX, 0);

        DrawActions();
        ActuallyDrawActions();

        sc2::renderer::Render();
    }

    void OnGameEnd() final {
        sc2::renderer::Shutdown();
    }
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
    coordinator.AddCommandLine("-eglpath /usr/lib/nvidia-367/libEGL.so");
#endif

    Replay replay_observer;
    coordinator.AddReplayObserver(&replay_observer);

    while (coordinator.Update()) {
        sc2::SleepFor(10);
    }
    while (!sc2::PollKeyPress());

    return 0;
}

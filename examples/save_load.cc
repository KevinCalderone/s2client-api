#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2renderer/sc2_renderer.h"

#include "sc2utils/sc2_manage_process.h"

#include <iostream>

using namespace sc2;

#define USE_SOFTWARE_RENDERING 0

const int kMapX = 800;
const int kMapY = 600;
const int kMiniMapX = 300;
const int kMiniMapY = 300;

class WorkerRushBot : public Agent {
public:
    virtual void OnGameStart() final {
        sc2::renderer::Initialize("Rendered", 50, 50, kMiniMapX + kMapX, std::max(kMiniMapY, kMapY));
    }

    void CenterCamera(std::vector<const Unit*> units) {
        if (units.size() == 0)
            return;

        Point2D center;
        for (auto unit : units) {
            center += unit->pos;
        }
        center /= units.size();

        Debug()->DebugMoveCamera(center);
        Debug()->SendDebug();
    }

    void RenderGame() {
        const SC2APIProtocol::Observation* observation = Observation()->GetRawObservation();
        const SC2APIProtocol::ObservationRender& render = observation->render_data();

        const SC2APIProtocol::ImageData& minimap = render.minimap();
        sc2::renderer::ImageRGB(&minimap.data().data()[0], minimap.size().x(), minimap.size().y(), 0, std::max(kMiniMapY, kMapY) - kMiniMapY);

        const SC2APIProtocol::ImageData& map = render.map();
        sc2::renderer::ImageRGB(&map.data().data()[0], map.size().x(), map.size().y(), kMiniMapX, 0);

        sc2::renderer::Render();
    }

    void Save () {
        // We haven't added proper support for this yet. This is a hack
        // to manually call into the underlying protobuf interface.
        GameRequestPtr request = Control()->Proto().MakeRequest();
        request->mutable_quick_save();
        if (!Control()->Proto().SendRequest(request)) {
            return;
        }

        Control()->WaitForResponse();
    }

    void Load () {
        // We haven't added proper support for this yet. This is a hack
        // to manually call into the underlying protobuf interface.
        GameRequestPtr request = Control()->Proto().MakeRequest();
        request->mutable_quick_load();
        if (!Control()->Proto().SendRequest(request)) {
            return;
        }

        Control()->WaitForResponse();
    }

    virtual void OnStep() final {
        const ObservationInterface* obs = Observation();
        
        // Worker rush the enemy!
        //
        auto enemy_base = obs->GetGameInfo().enemy_start_locations[0];
        const auto& workers = obs->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
        for (auto unit : workers) {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_base);
        }
        CenterCamera(workers);

        // Create a savepoint as we arrive at the enemy base
        //
        if (!has_save) {
            const auto& enemies = obs->GetUnits(Unit::Alliance::Enemy, IsVisible());
            if (enemies.size() > 0) {
                Save();
                has_save = true;
            }
        }
        
        // All our workers died... Load the savepoint and try again.
        //
        if (has_save && workers.size() == 0) {
            Load();
        }

        RenderGame();
    };

    virtual void OnGameEnd() final {
        sc2::renderer::Shutdown();
    };

private:
    bool has_save = false;
};

//*************************************************************************************************
int main(int argc, char* argv[]) {
    sc2::Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv)) {
        return 1;
    }

    sc2::RenderSettings settings(kMapX, kMapY, kMiniMapX, kMiniMapY);
    coordinator.SetRender(settings);

#if defined(__linux__)
#if USE_SOFTWARE_RENDERING
    coordinator.AddCommandLine("-osmesapath /usr/lib/x86_64-linux-gnu/libOSMesa.so.8");
#else
    coordinator.AddCommandLine("-eglpath /usr/lib/nvidia-384/libEGL.so");
#endif
#endif

    WorkerRushBot bot;
    coordinator.SetParticipants({
        CreateParticipant(sc2::Race::Terran, &bot),
        CreateComputer(sc2::Race::Protoss)
    });

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    while (coordinator.Update());
    while (!sc2::PollKeyPress());

    return 0;
}

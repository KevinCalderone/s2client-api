#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"

#include "sc2utils/sc2_manage_process.h"

#include <iostream>

using namespace sc2;

class WorkerRushBot : public Agent {
public:
    virtual void OnGameStart() final {
    };

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

    void Save () {
        GameRequestPtr request = Control()->Proto().MakeRequest();
        request->mutable_quick_save();
        if (!Control()->Proto().SendRequest(request)) {
            return;
        }

        Control()->WaitForResponse();
    }

    void Load () {
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
        auto& workers = obs->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
        for (auto unit : workers) {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_base);
        }
        CenterCamera(workers);

        // Create a savepoint as we arrive at the enemy base
        //
        if (!has_save) {
            auto& enemies = obs->GetUnits(Unit::Alliance::Enemy, IsVisible());
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
    };

    virtual void OnGameEnd() final {
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

    WorkerRushBot bot;
    coordinator.SetParticipants({
        CreateParticipant(sc2::Race::Terran, &bot),
        CreateComputer(sc2::Race::Protoss)
    });

    // Start the game.
    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    // Step forward the game simulation.
    while (coordinator.Update());

    return 0;
}

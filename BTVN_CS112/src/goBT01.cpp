// g++ -static -Os -Wall -o demo.exe ./tools/demo.cpp -lgdi32 && sudo ./demo.exe

#include "../libs/ModuleBT.hpp"
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

using namespace std;

struct Navigator {
    // Parameters (tune if necessary)
    const double SPACING = 150.0;             // grid spacing for exploration
    const double CLUSTER_RADIUS = 200.0;      // radius to consider sightings part of same cluster
    const int MIN_SIGHTINGS_FOR_ZONE = 3;     // how many sightings (near each other) -> confirm zone
    const double EXPLOIT_RADIUS = 120.0;      // radius to move around zone when exploiting
    const int ZONE_STALE_THRESHOLD = 12;      // number of steps without sighting -> abandon zone
    const int MAX_RECENT_SIGHTINGS = 100;     // keep recent sightings only

    // exploration spiral state (rectangular spiral)
    long long spiral_leg_len = 1;   // how many steps to take in current leg
    int spiral_dir = 0;             // 0:right,1:up,2:left,3:down
    long long spiral_leg_progress = 0;
    long long spiral_step_count = 0;
    // we need an origin for exploration (set to the first position seen)
    bool have_explore_origin = false;
    double explore_origin_x = 0.0, explore_origin_y = 0.0;
    long long spiral_x = 0, spiral_y = 0; // integer grid coordinates (multiples of SPACING)

    // sightings of boss 36
    deque<pair<double,double>> sightings;

    // special zone
    bool has_zone = false;
    double zone_x = 0.0, zone_y = 0.0;
    int steps_since_last_zone_sighting = 0;

    // exploitation pattern
    double exploit_angle = 0.0;
    const double EXPLOIT_ANGLE_STEP = M_PI / 6.0; // 30 degrees per step

    Navigator() {}

    // Compute next exploration grid point (deterministic rectangular spiral)
    pair<double,double> nextExplorationPoint() {
        if (!have_explore_origin) {
            // if we don't have an origin, we will start at (0,0) later to be set by caller
            // but we still create coordinates relative to origin
            have_explore_origin = true; // caller should have set explore_origin before first call
            spiral_x = 0; spiral_y = 0;
            spiral_leg_len = 1;
            spiral_dir = 0;
            spiral_leg_progress = 0;
            spiral_step_count = 0;
            return {explore_origin_x, explore_origin_y};
        }

        // we move on integer grid (spiral_x, spiral_y), then multiply by SPACING and add explore_origin
        // move one step in current direction
        if (spiral_dir == 0) spiral_x += 1;       // right
        else if (spiral_dir == 1) spiral_y += 1;  // up
        else if (spiral_dir == 2) spiral_x -= 1;  // left
        else spiral_y -= 1;                       // down

        spiral_leg_progress++;
        if (spiral_leg_progress >= spiral_leg_len) {
            // switch direction
            spiral_leg_progress = 0;
            spiral_dir = (spiral_dir + 1) % 4;
            // after two direction changes, increase leg length
            spiral_step_count++;
            if (spiral_step_count % 2 == 0) spiral_leg_len++;
        }

        double nx = explore_origin_x + spiral_x * SPACING;
        double ny = explore_origin_y + spiral_y * SPACING;
        return {nx, ny};
    }

    // add a sighting and maintain recent sightings buffer
    void recordSighting(double x, double y) {
        sightings.emplace_back(x, y);
        if ((int)sightings.size() > MAX_RECENT_SIGHTINGS) sightings.pop_front();
    }

    // Try to detect cluster: check last sightings for cluster of size >= MIN_SIGHTINGS_FOR_ZONE
    bool tryDetectZone() {
        int n = sightings.size();
        if (n < MIN_SIGHTINGS_FOR_ZONE) return false;

        // We'll check each sighting as center, count neighbours within CLUSTER_RADIUS
        for (int i = max(0, n - MIN_SIGHTINGS_FOR_ZONE - 10); i < n; ++i) {
            int count = 0;
            double sx = 0, sy = 0;
            for (int j = max(0, n - MAX_RECENT_SIGHTINGS); j < n; ++j) {
                double dx = sightings[j].first - sightings[i].first;
                double dy = sightings[j].second - sightings[i].second;
                if (dx*dx + dy*dy <= CLUSTER_RADIUS*CLUSTER_RADIUS) {
                    count++;
                    sx += sightings[j].first;
                    sy += sightings[j].second;
                }
            }
            if (count >= MIN_SIGHTINGS_FOR_ZONE) {
                zone_x = sx / count;
                zone_y = sy / count;
                has_zone = true;
                steps_since_last_zone_sighting = 0;
                // reset exploit angle to systematically sweep
                exploit_angle = 0.0;
                return true;
            }
        }
        return false;
    }

    pair<double,double> getNext(double x, double y, int lastBossID) {
        // initialize origin if first call
        if (!have_explore_origin) {
            have_explore_origin = true;
            explore_origin_x = x;
            explore_origin_y = y;
            spiral_x = 0; spiral_y = 0;
            spiral_leg_len = 1;
            spiral_dir = 0;
            spiral_leg_progress = 0;
            spiral_step_count = 0;
        }

        // If a sighting occurred
        if (lastBossID == 36) {
            recordSighting(x, y);
            steps_since_last_zone_sighting = 0;
            // if we already had a zone, update zone center slightly towards this sighting (moving average)
            if (has_zone) {
                // incorporate new sighting in centroid to adapt
                zone_x = (zone_x * 7 + x) / 8.0;
                zone_y = (zone_y * 7 + y) / 8.0;
            } else {
                // attempt detect zone now
                tryDetectZone();
            }
        } else {
            // no sighting this step
            if (has_zone) {
                steps_since_last_zone_sighting++;
                if (steps_since_last_zone_sighting > ZONE_STALE_THRESHOLD) {
                    // zone is stale -> abandon and reset
                    has_zone = false;
                    // optionally shrink sightings to recent ones to allow new detection
                    while (sightings.size() > MIN_SIGHTINGS_FOR_ZONE) sightings.pop_front();
                }
            }
        }

        // If we have a special zone, exploit it
        if (has_zone) {
            // compute exploit point on a deterministic circle (advance angle each call)
            double tx = zone_x + EXPLOIT_RADIUS * cos(exploit_angle);
            double ty = zone_y + EXPLOIT_RADIUS * sin(exploit_angle);

            // advance angle deterministically
            exploit_angle += EXPLOIT_ANGLE_STEP;
            if (exploit_angle > 2*M_PI) exploit_angle -= 2*M_PI;
            return {tx, ty};
        }

        // Otherwise explore: next point in spiral grid
        auto p = nextExplorationPoint();
        return p;
    }
};

// The user-level function as requested
pair<float,float> getNextPoint(float x, float y, int lastBossID) {
    static Navigator nav; // persistent between calls
    auto p = nav.getNext((double)x, (double)y, lastBossID);
    return { (float)p.first, (float)p.second };
}


inline pair<float,float> Module::T1(float x, float y, int lastBossID) {
	return getNextPoint(x, y, lastBossID);
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------



Memory::DWORD pid = 0;

const std::vector<std::string> targetAutoAim = {
    ".*gameplay.*", // 任意奖励箱 gameplay/...
    ".*5star.*",
};

const std::vector<std::string> noTargetAutoAim = {
    // ".*chest_quest_worldboss.*",       // 利维坦奖励箱
    "radiantprism",                    // 天空光辉碎片
    ".*quest_spawn_trigger_radiant.*", // 天空黑暗之心开关
    ".*goodkarma.*",                   // 善业NPC
    ".*pet.*",                         // 任意宠物
    ".*portal.*",                      // 传送门
    ".*abilities.*",                   // 任意投射物
    ".*placeable.*",                   // 任意放置物
    // 其他未知项
    ".*services.*",
    ".*client.*",
    ".*september.*",
};

const std::vector<std::string> targetFollow = []
{auto tmp = targetAutoAim;tmp.insert(tmp.end(),{
    ".*viking_npc.*",                  // 稀有NPC
    ".*quest.*trigger.*",              // 任意开关
});return tmp; }();

const std::vector<std::string> noTargetFollow = noTargetAutoAim;

std::atomic<bool> findTarget = false;
void FindTarget(const bool &targetBoss = true,
                const bool &targetPlant = false,
                const bool &targetNormal = false,
                const std::vector<std::string> &targets = {},
                const std::vector<std::string> &noTargets = {},
                const uint32_t &range = 45)
{
    Game game(pid);
    game.UpdateAddress();
    std::unique_ptr<Game::World::Entity> target = nullptr;
    game.data.player.UpdateAddress();
    auto UpdateAddress = [&game]()
    {
        game.data.player.data.coord.UpdateAddress();
        game.data.player.data.camera.UpdateAddress();
        game.data.player.data.coord.data.x.UpdateAddress();
        game.data.player.data.coord.data.y.UpdateAddress();
        game.data.player.data.coord.data.z.UpdateAddress();
        game.data.player.data.camera.data.v.UpdateAddress();
        game.data.player.data.camera.data.h.UpdateAddress();
    };
    Module::Target oldTarget;
    std::vector<Module::Target> blackTargetList;
    std::chrono::steady_clock::time_point findTime;
    while (findTarget.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        UpdateAddress();
        target = Module::FindTarget(game, targetBoss, targetPlant, targetNormal, targets, noTargets, range, blackTargetList);
        if (!target)
        {
            continue;
        }
        std::cout << "\n";
        printf("--> Level: %02d X: %+07.1f Y: %+07.1f Z: %+07.1f Name: %s\n",
               target->data.level.UpdateData().data % 100,
               target->data.x.UpdateData().data,
               target->data.y.UpdateData().data,
               target->data.z.UpdateData().data,
               target->data.name.UpdateData(128).data.c_str());
        std::cout << "\n";
        oldTarget.name = target->data.name.data;
        oldTarget.level = target->data.level.data;
        oldTarget.health = target->data.health.data;
        oldTarget.x = target->data.x.data;
        oldTarget.y = target->data.y.data;
        oldTarget.z = target->data.z.data;

        findTime = std::chrono::steady_clock::now();
        while (CalculateDistance(
                   game.data.player.data.coord.data.x.UpdateData().data,
                   game.data.player.data.coord.data.y.UpdateData().data + Module::aimOffset.first,
                   game.data.player.data.coord.data.z.UpdateData().data,
                   target->data.x.UpdateData().data,
                   target->data.y.UpdateData().data + Module::aimOffset.second,
                   target->data.z.UpdateData().data) <= range &&
               target->data.isDeath.UpdateData().data && findTarget.load())
        {
            UpdateAddress();
            auto health = target->data.health.UpdateData().data;
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - findTime).count() >= Module::entityTimeout)
            {
                blackTargetList.push_back(oldTarget);
                break;
            }

            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - findTime).count() * 5 >= Module::entityTimeout ||
                target->data.health.UpdateData().data < 1 ||
                health < 1 ||
                (target->data.x.data < 1 && target->data.y.data < 1 && target->data.z.data < 1))
            {
                const auto &entitys = game.data.world.UpdateAddress().UpdateData().data.entitys;
                if (std::find(entitys.begin(), entitys.end(), *target) == entitys.end())
                    break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void FindAobScan(const char *signature)
{
    Game game(pid);
    for (auto i : game.AobScan(signature, true))
        printf("%08X\n", i);
}

void AutoScan()
{
    printf("注意: 请以管理员权限运行\n");
    printf("按任意键退出\n");

    FunctionOn(pid, "SetByPass", "1", false);
    FunctionOn(pid, "SetNoClip", "1", false);
    FunctionOn(pid, "SetAutoAttack", "300|1000", false);

    auto vector2string = [](const std::vector<std::string> &vector)
    {
        std::string tmp = "";
        for (const std::string &str : vector)
            tmp += str + ",";
        return tmp;
    };

    char buffer[1024];

    sprintf(buffer, "%s|%s|%s|%d|%d|%u|%u",
            " ",                                   // 跟随玩家名单
            vector2string(targetFollow).c_str(),   // 跟随实体名单
            vector2string(noTargetFollow).c_str(), // 不跟随实体名单
            true,                                  // 跟随boss实体
            true,                                  // 扫图模式
            50,                                    // 跟随速度
            50                                     // 跟随频率
    );
    FunctionOn(pid, "FollowTarget", buffer, false);

    sprintf(buffer, "%d|%d|%d|%s|%s|%u|%u",
            true,                                   // 瞄准boss实体
            false,                                  // 瞄准植物实体
            false,                                  // 瞄准普通怪物实体
            vector2string(targetAutoAim).c_str(),   // 瞄准实体名单
            vector2string(noTargetAutoAim).c_str(), // 不瞄准实体名单
            45,                                     // 瞄准范围
            50                                      // 瞄准频率
    );
    FunctionOn(pid, "AutoAim", buffer, false);

    findTarget.store(true);
    new std::thread(
        FindTarget, true, false, false,
        targetAutoAim,
        noTargetAutoAim,
        9999);
    atexit([]()
           {
     Module::StopAll(pid);
     findTarget.store(false); 
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); });
    getchar();
}

void WhatItem()
{
    findTarget.store(true);
    new std::thread(
        FindTarget, true, false, false,
        std::vector<std::string>{".*"},
        std::vector<std::string>{},
        5);
    atexit([]()
           { findTarget.store(false); });
    getchar();
}

int main(int argc, char *argv[])
{
    SetConsoleOutputCP(65001);
    pid = argc > 1 ? std::stol(argv[1]) : Memory::GetProcessPid("Trove.exe")[0];
    AutoScan();
    return 0;
}
// g++ -static -Os -Wall -o demo.exe ./tools/demo.cpp -lgdi32 && sudo ./demo.exe

#include "../libs/ModuleBT.hpp"
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

using namespace std;
inline pair<float,float> Module::T1(float x, float y, int lastBossID) {
	using Point = pair<double, double>;

    static const auto dist = [](const Point &a, const Point &b) {
        return hypot(a.first - b.first, a.second - b.second);
    };
    static const auto randDouble = [](double a, double b) {
        return a + (b - a) * (rand() / (double)RAND_MAX);
    };


    enum AgentState { SEARCHING, FARMING, RETURNING };
    const double EXPLORATION_STEP = 300.0;
    const double MIN_FARMING_STEP = 90.0;
    const double MAX_FARMING_STEP = 160.0;
    const int LOST_THRESHOLD = 5;

    struct StaticData {
        AgentState currentState = SEARCHING;
        vector<Point> specialBossLocations;
        Point zoneCentroid = {0.0, 0.0};
        double adaptiveZoneRadius = 0.0;
        int stepsWithoutKill = 0;
        int spiralDirection = 0;
        int spiralLegLength = 1;
        int stepsOnCurrentLeg = 0;
        int turnsMade = 0;
    };
    static StaticData mem;

    Point cur{x, y};

    if (lastBossID == 36) {
        if (mem.currentState == SEARCHING) { cout << "[LOG] Tim thay khu vuc dac biet! Chuyen sang FARMING.\n"; }
        mem.currentState = FARMING;
        mem.stepsWithoutKill = 0;
        mem.specialBossLocations.push_back(cur);
        
        if (mem.specialBossLocations.size() > 1) {
            double sumX = 0, sumY = 0;
            for (const auto& p : mem.specialBossLocations) { sumX += p.first; sumY += p.second; }
            mem.zoneCentroid.first = sumX / mem.specialBossLocations.size();
            mem.zoneCentroid.second = sumY / mem.specialBossLocations.size();

            vector<double> distances;
            for (const auto& p : mem.specialBossLocations) { distances.push_back(dist(mem.zoneCentroid, p)); }
            double dist_mean = accumulate(distances.begin(), distances.end(), 0.0) / distances.size();
            double sq_sum = 0.0;
            for(const double d : distances) { sq_sum += (d - dist_mean) * (d - dist_mean); }
            double std_dev = sqrt(sq_sum / distances.size());
            mem.adaptiveZoneRadius = dist_mean + 2.0 * std_dev;
        } else {
             mem.zoneCentroid = mem.specialBossLocations[0];
             mem.adaptiveZoneRadius = MAX_FARMING_STEP * 1.5;
        }
    } else if (lastBossID > 0) {
        mem.stepsWithoutKill = 0;
    } else {
        mem.stepsWithoutKill++;
    }

    if (mem.currentState == FARMING && mem.stepsWithoutKill >= LOST_THRESHOLD) { mem.currentState = RETURNING; }
    if (mem.currentState == RETURNING && (lastBossID > 0 || dist(cur, mem.zoneCentroid) < MAX_FARMING_STEP)) {
        mem.currentState = FARMING;
        mem.stepsWithoutKill = 0;
    }

    Point nextPoint;
    switch (mem.currentState) {
        case SEARCHING: {
            double dx = 0, dy = 0;
            if (mem.spiralDirection == 0) dx = 1; else if (mem.spiralDirection == 1) dy = 1;
            else if (mem.spiralDirection == 2) dx = -1; else dy = -1;
            nextPoint = { cur.first + EXPLORATION_STEP * dx, cur.second + EXPLORATION_STEP * dy };
            mem.stepsOnCurrentLeg++;
            if (mem.stepsOnCurrentLeg >= mem.spiralLegLength) {
                mem.stepsOnCurrentLeg = 0;
                mem.spiralDirection = (mem.spiralDirection + 1) % 4;
                mem.turnsMade++;
                if (mem.turnsMade == 2) { mem.turnsMade = 0; mem.spiralLegLength++; }
            }
            break;
        }
        case FARMING: {
            double ang = randDouble(0, 2 * M_PI), step = randDouble(MIN_FARMING_STEP, MAX_FARMING_STEP);
            double dx_rand = cos(ang), dy_rand = sin(ang);
            double dist_from_center = dist(cur, mem.zoneCentroid);
            double dx_center = mem.zoneCentroid.first - cur.first, dy_center = mem.zoneCentroid.second - cur.second;
            if (dist_from_center > 1e-6) { dx_center /= dist_from_center; dy_center /= dist_from_center; }
            double pull_factor = 0.0;
            if (mem.adaptiveZoneRadius > 0) {
                 double ratio = min(1.0, dist_from_center / (mem.adaptiveZoneRadius * 1.5));
                 pull_factor = pow(ratio, 2);
            }
            double final_dx = dx_rand * (1.0 - pull_factor) + dx_center * pull_factor;
            double final_dy = dy_rand * (1.0 - pull_factor) + dy_center * pull_factor;
            double final_len = hypot(final_dx, final_dy);
            if (final_len > 1e-6) { final_dx /= final_len; final_dy /= final_len; }
            nextPoint = { cur.first + step * final_dx, cur.second + step * final_dy };
            break;
        }
        case RETURNING: {
            double dx = mem.zoneCentroid.first - cur.first, dy = mem.zoneCentroid.second - cur.second;
            double len = hypot(dx, dy);
            if (len < 50.0) {
                 mem.currentState = FARMING; mem.stepsWithoutKill = 0;
                 double ang = randDouble(0, 2 * M_PI);
                 nextPoint = { cur.first + MAX_FARMING_STEP*cos(ang), cur.second + MAX_FARMING_STEP*sin(ang) };
            } else {
                 double return_step = min(len, EXPLORATION_STEP * 1.5);
                 nextPoint = { cur.first + return_step * dx / len, cur.second + return_step * dy / len };
            }
            break;
        }
    }
    return nextPoint;
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------



Memory::DWORD pid = 0;

const std::vector<std::string> targetAutoAim = {
    ".*gameplay.*", // 任意奖励箱 gameplay/...
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
// g++ -static -Os -Wall -o demo.exe ./tools/demo.cpp -lgdi32 && sudo ./demo.exe

#include "../libs/ModuleBT.hpp"
using namespace std;
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

// Trạng thái chung: spiral lục giác quanh anchor hiện tại; dùng cho cả 2 pha
static bool phase2 = false;   // đã gặp Boss 36 hay chưa
static bool hexInit = false;  // đã khởi tạo spiral hay chưa
static double anchorX = 0.0, anchorY = 0.0; // tâm spiral hiện hành

// Trạng thái spiral
static long long hexRing = 0;   // vòng hiện tại (1,2,3,...)
static int hexSide = 0;         // cạnh hiện tại [0..5]
static int hexStepOnSide = 0;   // số bước đã đi trên cạnh hiện tại [0..hexRing-1]
static bool hexAtStartOfRing = true; // cần bước 1 bước để ra đầu vòng
static double hx = 0.0, hy = 0.0;    // vị trí hiện tại

constexpr double SQRT3 = 1.7320508075688772935;
constexpr double HEX_STEP = 100.0;   // độ dài mỗi bước
// Ngưỡng để xác định đã "bám lại" đúng quỹ đạo (tránh sai số số thực và
// cho phép kéo về quỹ đạo trước khi đi tiếp). Chọn nhỏ hơn bước hex.
constexpr double REJOIN_EPS = HEX_STEP * 0.05; // 5 đơn vị
constexpr double REJOIN_EPS2 = REJOIN_EPS * REJOIN_EPS;
// 6 hướng của lưới lục giác (cách 60°): 0°,60°,120°,180°,240°,300°
static const double DIRX[6] = { 1.0,  0.5, -0.5, -1.0, -0.5,  0.5 };
static const double DIRY[6] = { 0.0,  SQRT3/2,  SQRT3/2, 0.0, -SQRT3/2, -SQRT3/2 };

inline void initSpiral(double ax, double ay) {
    anchorX = ax; anchorY = ay;
    hexRing = 1; hexSide = 0; hexStepOnSide = 0;
    hexAtStartOfRing = true;
    hx = anchorX; hy = anchorY;
    hexInit = true;
}

inline pair<double,double> nextHexStep() {
    if (hexAtStartOfRing) {
        // Bước ra đỉnh bắt đầu của vòng hiện tại theo hướng dir4 (240°)
        hx += HEX_STEP * DIRX[4];
        hy += HEX_STEP * DIRY[4];
        hexAtStartOfRing = false;
    } else {
        // Đi tiếp hexRing bước dọc 6 cạnh
        hx += HEX_STEP * DIRX[hexSide];
        hy += HEX_STEP * DIRY[hexSide];
        ++hexStepOnSide;
        if (hexStepOnSide == hexRing) {
            hexStepOnSide = 0;
            ++hexSide;
            if (hexSide == 6) {
                hexSide = 0;
                ++hexRing;           // sang vòng lớn hơn
                hexAtStartOfRing = true; // lượt tới sẽ bước ra đầu vòng mới
            }
        }
    }
    return {hx, hy};
}

inline pair<float,float> Module::T1(float x, float y, int lastBossID) {
    // Quy tắc mới:
    // - Nếu boss != 0: không di chuyển theo quỹ đạo, chỉ trả về {x,y}.
    //   Trường hợp gặp Boss 36 lần đầu vẫn cập nhật trạng thái nội bộ (reset anchor),
    //   nhưng lượt này vẫn giữ nguyên {x,y}.
    if (lastBossID != 0) {
        if (!phase2 && lastBossID == 36) {
            phase2 = true;
            initSpiral(x, y); // cập nhật quỹ đạo định sẵn tại vị trí gặp boss 36
        }
        return {x, y};
    }

    // boss == 0: di chuyển theo quỹ đạo định sẵn
    // Khởi tạo spiral lần đầu (neo tại vị trí đầu tiên không có boss)
    if (!hexInit) {
        initSpiral(x, y);
        phase2 = false;
    }

    // Nếu hiện tại đã lệch khỏi quỹ đạo, ưu tiên kéo về đúng điểm quỹ đạo (hx,hy)
    // Chỉ khi đã về đúng (trong ngưỡng) mới đi tiếp điểm kế tiếp trên quỹ đạo
    double dx = (double)x - hx;
    double dy = (double)y - hy;
    double d2 = dx*dx + dy*dy;
    if (d2 > REJOIN_EPS2) {
        return {(float)hx, (float)hy};
    }

    auto [nx, ny] = nextHexStep();
    return {(float)nx, (float)ny};
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
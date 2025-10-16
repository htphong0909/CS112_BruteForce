// g++ -static -Os -Wall -o demo.exe ./tools/demo.cpp -lgdi32 && sudo ./demo.exe

#include "../libs/ModuleBT.hpp"
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

using namespace std;

// ====== Cấu hình spiral ======
const float SQUARE_STEP = 150.0f;
const float HEX_RADIUS  = 50.0f;

// ====== Trạng thái toàn cục ======
bool inSpecial = false;
pair<float,float> center = {0, 0};

// Biến spiral vuông (giữ nguyên)
int sq_dir = 0, sq_len = 1, sq_cnt = 0;
float sq_x = 0, sq_y = 0;

// THAY ĐỔI: Xóa các biến spiral lục giác toàn cục cũ
// int hex_ring = 0;
// int hex_dir = 0;
// int hex_step = 0;
// int hex_q = 0, hex_r = 0;
// const pair<int,int> hex_dirs[6] = ... // Cái này cũng không cần nữa

// ====== Chuyển từ axial -> tọa độ thế giới ======
std::pair<float, float> axial_to_world(int q, int r, float hex_radius) {
    float x = hex_radius * (3.0f / 2.0f * q);
    float y = hex_radius * (sqrt(3.0f) / 2.0f * q + sqrt(3.0f) * r);
    return {x, y};
}

pair<float,float> nextSquareSpiral() {
    static const float dx[4] = {SQUARE_STEP, 0, -SQUARE_STEP, 0};
    static const float dy[4] = {0, SQUARE_STEP, 0, -SQUARE_STEP};

    sq_x += dx[sq_dir];
    sq_y += dy[sq_dir];
    sq_cnt++;

    if (sq_cnt == sq_len) {
        sq_cnt = 0;
        sq_dir = (sq_dir + 1) % 4;
        if (sq_dir % 2 == 0) sq_len++;
    }
    return {sq_x, sq_y};
}


class HexSpiralGenerator {
private:
    bool is_center;       
    int q, r;         
    int ring;            
    int side;             
    int step;               

    const std::vector<std::pair<int, int>> AXIAL_DIRECTIONS = {
        {-1, 0}, {-1, +1}, {0, +1}, {+1, 0}, {+1, -1}, {0, -1}
    };

public:
    HexSpiralGenerator() {
        reset();
    }

    void reset() {
        is_center = true;
        ring = 1;
        side = 0;
        step = 0;
        q = 1;
        r = -1;
    }

    std::pair<int, int> next() {
        if (is_center) {
            is_center = false;
            return {0, 0};
        }

        std::pair<int, int> current_pos = {q, r};

        auto [dq, dr] = AXIAL_DIRECTIONS[side];
        q += dq;
        r += dr;
        step++;

        if (step % ring == 0) {
            side = (side + 1) % 6; 
            if (side == 0) {
                ring++; 
                q=ring;
                r=-ring;
                step=0;
            }
        }

        return current_pos;
    }
}; 
HexSpiralGenerator spiral_gen;
pair<float,float> getNextPoint(float x, float y, int lastBossID) {
    static pair<float,float> current = {0, 0};
    static int iscenter = 1;
    if(iscenter)
    {
        current = {x,y};
        iscenter = 0;
        return current;
    }
    if (!inSpecial) {
        if (lastBossID == 36) {
            inSpecial = true;
            center = {x, y};    // Lưu lại tâm của vùng đặc biệt
            current = center;   // Vị trí đầu tiên trong vùng đặc biệt chính là tâm

            // THAY ĐỔI: Reset generator để bắt đầu vòng xoắn ốc mới từ tâm
            // THAY ĐỔI: Gọi next() một lần để "tiêu thụ" điểm tâm {0,0}.
            // Lần gọi getNextPoint tiếp theo sẽ bắt đầu từ điểm đầu tiên của vòng 1.
            spiral_gen.next();

            return current; // Trả về vị trí tâm ngay lập tức
        } else if (lastBossID == 0) {
            current = nextSquareSpiral();
        }
        // else: giữ nguyên `current` nếu hạ boss thường
        return current;
    }

    // Pha 2: Duyệt spiral lục giác quanh center
    if (lastBossID != 0) {
        return current; // Giữ nguyên vị trí nếu đang hạ boss
    }

    // Lấy tọa độ axial tiếp theo từ generator
    pair<int,int> next_axial_coords = spiral_gen.next();
    // Chuyển đổi sang tọa độ thế giới (tương đối so với tâm)
    auto [offset_x, offset_y] = axial_to_world(next_axial_coords.first, next_axial_coords.second, HEX_RADIUS);

    // Tính tọa độ tuyệt đối
    current.first = center.first + offset_x;
    current.second = center.second + offset_y;

    return current;
}

// Ý tưởng thuật toán:
//
// Phần 1: Tìm vùng đặc biệt:
// - Duyệt theo hình xoắn ốc với mỗi bước cách nhau 150 đơn vị.
// - Nếu hạ được boss ID 36, chuyển sang phần 2.
// - Nếu hạ được boss thường (ID != 0), giữ nguyên vị trí hiện tại để tiêu diệt hết boss trong vùng.
// - Nếu không hạ được boss, tiếp tục di chuyển theo spiral.
// Phần 2: Duyệt vùng đặc biệt:
// - Bắt đầu từ vị trí hạ được boss ID 36, chia bản đồ thành các hình lục giác đều với bán kính 50 đơn vị.
// - Lần lượt duyệt các vùng lục giác theo hình xoắn ốc với tâm là vị trí 36 đầu tiên.
// - Nếu hạ được boss thường, giữ nguyên vị trí để tiêu diệt hết boss trong vùng.
// - Nếu hạ được boss ID 36 khác, giữ nguyên vị trí và tiếp tục duyệt các vùng lục giác tiếp theo.
// - Nếu không hạ được boss, tiếp tục di chuyển theo spiral lục giác.

// Đảm bảo thuật toán:
// - Phần 1 Khả năng cao tìm được ít nhất một vùng đặc biệt nếu tồn tại
// - Phần 2 đảm bảo tiêu diệt mọi boss trong vùng
// - Vì đề đảm bảo bản đồ mỗi vùng là "rất lớn" nên việc duyệt phần 1 khả năng cao là không miss vùng đặc biệt và nếu đã tìm được vùng đặc biệt
//   thì việc duyệt phần 2 sẽ bao phủ toàn bộ vùng đặc biệt.
// - Việc giữ nguyên vị trí khi hạ boss thường đảm bảo không bỏ sót boss trong vùng đặc biệt
// Chia ô theo lục giác đều đảm bảo việc bao phủ vùng là tối ưu nhất với số bước di chuyển ít nhất,
// diện tích bị thừa mỗi bước chỉ khoảng 17% so với hình tròn bao quanh lục giác.
// int main()
// {
//     pair<float,float> pos = {0,0};
//     int lastBossID = 0;
//     for(int i=1;i<=100;i++)
//     {
//         pos = getNextPoint(pos.first, pos.second, lastBossID);
//         cout << fixed << setprecision(2) << pos.first << " " << pos.second << "\n";
//         cin>> lastBossID;
//         if(lastBossID !=0)  
//             cin >> pos.first >> pos.second; 
//     }
// }

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
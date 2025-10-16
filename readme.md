# Đề bài BTVN

Trong một trò chơi online nhiều người chơi, thế giới được biểu diễn trên mặt phẳng 2D với hệ trục tọa độ Oxy. Trò chơi bao gồm nhiều khu vực khác nhau, mỗi khu vực có diện tích rất lớn. Trong mỗi khu vực tồn tại nhiều dungeon, và trong mỗi dungeon sẽ có một con quái đặc biệt gọi là boss.

Mỗi đối tượng trong game (nhân vật, boss, dungeon, …) được xác định bởi tọa độ hai số thực `(x, y)`.

Bạn có một chương trình tự động điều khiển nhân vật, hoạt động theo quy trình:

1. **Quét xung quanh:** Nhân vật quét khu vực bán kính ≤ 50 quanh vị trí hiện tại.
2. **Xử lý kết quả quét:**

   * Nếu phát hiện có boss, nhân vật di chuyển (di chuyển khá nhanh, mất khoảng 1s nếu boss ở tối đa tầm) đến boss gần nhất và hạ gục nó; biến `lastBossID` được gán bằng ID của boss vừa bị hạ gục, sau đó quay lại bước 1.
   * Nếu không phát hiện boss, nhân vật giữ nguyên vị trí và `lastBossID = 0`.
3. **Xác định bước đi tiếp theo:**

   * Nếu nhân vật đang chưa đi tới bước tiếp theo thì tiếp tục đi tới; trong lúc đi có thể vừa đi vừa quét boss và nếu tìm thấy thì quay lại bước 1.
   * Nếu nhân vật đã đi tới thì gọi hàm `getNextPoint(x, y, lastBossID)` để lấy bước tiếp theo.

**Biết rằng:**

* Các dungeon được phân bố đều trong từng khu vực, cả ở trung tâm lẫn biên giới.
* Trung bình khoảng cách giữa các dungeon là 70 → 200; đôi khi vị trí có thể xa hơn do có dungeon đã bị người chơi khác hạ trước đó.
* Mỗi khu vực có mật độ dungeon và boss như nhau.
* **Đặc biệt:** Có một khu vực mà khoảng 63% số boss ở đó có `ID = 36` (thật ra là 50% 🐧), và boss có `ID = 36` chỉ xuất hiện duy nhất trong khu vực đó.

---

# Yêu cầu

Hãy xây dựng và mô tả thuật toán cho hàm `getNextPoint(x, y, lastBossID)` sao cho:

* Nhân vật có thể tìm ra và ở lại khai thác khu vực đặc biệt, dù bản đồ có diện tích rất lớn.
* Nhân vật hạ gục được nhiều boss nhất trong khu vực đặc biệt (không chỉ riêng boss `ID = 36`, mà mọi boss trong khu vực đó đều được tính).

---

# Lưu ý

* Khu vực đặc biệt đôi khi sẽ có dạng hình thù khá đặc biệt, có thể là một đa giác lõm.
* Liên hệ bạn Hà Thanh Phong (`24520024`) để được chỉnh sửa và chạy thử thực tế, xem cụ thể game.
* Các nhóm ưu tiên nộp trước **thứ 5 tuần sau** để bạn Phong có thể kịp tổng hợp các nhóm 🥳🥳😳



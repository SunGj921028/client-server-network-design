## NTNU 41147005S 傅靖嘉 - Project Phase 1

### How to use
- 先在 /code 底下執行 `make` 進行編譯 
- For server：
  - `./server <port_number>`
- For client：
  - `./client <IP_address> <port_number>`

### 使用範例（demo 影片使用）
- `./server 8080`
- `./client 0.0.0.0 8080`
- IP address 需合法，
  - 0.0.0.0 指在本機電腦上的 IP
- IP_address, port_number 請輸入數字

### 特殊事項
> 其實所有指令都可以當作 client 跟 server 的 communication
#### Authentication Features
(1) register <username> <password>
(2) login <username> <password>
(3) exit
- 輸入限制
  - 請忽略數字標題並按照格式打，不要直接輸入數字
  - username 跟 password 的長度最長為 50
  - 兩個 username 與 password 之間的空格輸入多個也沒關係，重點在於 register/login, username, password 三個不能少任何一個
  - username 和 password 的組成不限制（可以是中文英文符號數字之類的）
  - register, login, exit 都是 case insensitive
  
- 當今天是有 login 的狀態，不能直接 exit，要先登出（logout）
  - exit 會斷開 client 的連線，並終止 client 端的程式，但 server 會持續運作
  
- 不能有重複的帳號名稱
  
- 登入後若要註冊或登入別的帳號請先登出
  
- 使用者資料在註冊後將儲存在 `userInfo.txt` 裡面，就算連線結束資料也會保留

- 有提供一筆預設帳號密碼
  - username: s
  - password: s
  
---

#### Basic Server-Client Communication
(1) send <message>
(2) logout
- 當使用者登入帳號後，就只能照格式輸入要傳送的 message 或是輸入 logout 登出
  - 輸入 logout 登出後會回到登入前的頁面，connection 不會被終止
  - 這邊的訊息有加上輸入 exit 的防呆（要先 logout 才可以 exit，但把 exit 當作普通訊息傳送則例外）
- 單個訊息（上面輸入中的 message）的長度最長為 1024

- 提供 client 和 server 之間訊息的溝通與互動
  - 目前不支援 Multithread server

- 當有接受到訊息的時候，那筆訊息會以 "- - -" 開頭為形式，顯示在 terminal 上

- 每一個 client 的動作都會有訊息傳遞（send）跟接受（recv）

- 不管輸入訊息格式錯誤或不合理，訊息都會傳遞給 server，server 也會接收並回傳特定的訊息 

#### 檔案簡易說明
- `client.cpp`, `client.hpp`
  - client 相關的內容
- `server.cpp`, `server.hpp`
  - server 相關的內容
- `defAndFuc.hpp`
  - 一些 server 和 client 共用的東西和一些固定的 message
- `userInfo.txt`
  - 使用者資料紀錄，包含初始帳密
- `Makefile`

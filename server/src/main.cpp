#include <arpa/inet.h> // IP / 포트 / htons, ntohl
#include <cerrno>      // errno
#include <cstring>     // strerror()
#include <iostream>    // cout, cerr
#include <thread>      // std::thread
#include <unistd.h>    // close()
#include <vector>      // std::vector

// ─────────────────────────────────────────────────────────────
// 서버가 사용하는 패킷 헤더 형식
// ─────────────────────────────────────────────────────────────
// TCP는 그냥 바이트 스트림이기 때문에
// 이 구조체로 "이게 하나의 메시지" 라는 경계를 만들어준다.
#pragma pack(push, 1) // 패딩 제거
struct PacketHeader {
  uint32_t length; // 패킷 전체 길이 (헤더 + 바디)
  uint16_t type;   // 명령 종류 (PING, CHAT 등)
  uint16_t flags;  // 확장용 플래그
};
#pragma pack(pop)

// ─────────────────────────────────────────────────────────────
// 클라이언트 1명 전담 처리 함수
// ─────────────────────────────────────────────────────────────
void handleClient(int clientSock) {

  char buf[1024];               // 커널 TCP 수신 버퍼에서 읽어올 임시 공간
  std::vector<char> recvBuffer; // TCP 특성상 쪼개져 오는 데이터를 누적할 버퍼

  while (true) {

    // 커널 TCP 수신 버퍼 → 유저 공간
    int len = recv(clientSock, buf, sizeof(buf), 0);

    // 연결 종료 또는 에러
    if (len <= 0)
      break;

    // 받은 만큼 누적
    recvBuffer.insert(recvBuffer.end(), buf, buf + len);

    // ───────── 패킷 조립 루프 ─────────
    // 버퍼에 헤더 이상 데이터가 쌓이면 패킷으로 해석 시도
    while (recvBuffer.size() >= sizeof(PacketHeader)) {

      PacketHeader *header =
          reinterpret_cast<PacketHeader *>(recvBuffer.data());

      // 네트워크 바이트 → 호스트 바이트 변환
      uint32_t packetLen = ntohl(header->length);
      uint16_t type = ntohs(header->type);

      const uint32_t MAX_PACKET_SIZE = 4096;

      // 길이 변조 공격 차단
      if (packetLen > MAX_PACKET_SIZE) {
        std::cerr << "[SECURITY] oversized packet detected\n";
        close(clientSock);
        return;
      }

      // 아직 패킷 전체가 도착 안 했으면 대기
      if (recvBuffer.size() < packetLen)
        break;

      // ───────── 패킷 처리 ─────────
      switch (type) {
      case 0x0001:
        std::cout << "[PING PACKET]\n";
        break;
      case 0x0003:
        std::cout << "[CHAT PACKET]\n";
        break;
      default:
        std::cout << "[UNKNOWN PACKET]\n";
      }

      // 처리 완료한 패킷 제거
      recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + packetLen);
    }
  }

  close(clientSock); // 클라이언트 TCP 세션 종료
}

// ─────────────────────────────────────────────────────────────
// 서버 시작점
// ─────────────────────────────────────────────────────────────
int main() {

  // 1. TCP 소켓 생성 → 커널 TCP 엔진 활성화
  int serverSock = socket(AF_INET, SOCK_STREAM, 0);

  int opt = 1;
  setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // 2. 서버 주소 설정
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9000);
  addr.sin_addr.s_addr = INADDR_ANY;

  // 3. 포트 등록
  bind(serverSock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));

  // 4. 연결 대기 상태
  listen(serverSock, 128);

  std::cout << "Server running on port 9000\n";

  // 5. 접속자 수락 루프
  while (true) {
    int clientSock = accept(serverSock, nullptr, nullptr);
    std::thread(handleClient, clientSock).detach();
  }
}

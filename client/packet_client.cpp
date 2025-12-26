#include <arpa/inet.h> // htons, inet_pton : 네트워크 바이트 변환
#include <cstring>     // strlen()
#include <iostream>    // cout, cerr
#include <unistd.h>    // close()

// ─────────────────────────────────────────────────────────────
// 서버와 동일한 패킷 헤더 구조
// 이 구조체가 클라이언트·서버 간 공통 언어다.
// ─────────────────────────────────────────────────────────────
#pragma pack(push, 1)
struct PacketHeader {
  uint32_t length; // 전체 패킷 길이 (헤더 + 바디)
  uint16_t type;   // 패킷 종류 (CHAT = 0x0003)
  uint16_t flags;  // 확장용
};
#pragma pack(pop)

int main() {

  // 1. TCP 소켓 생성
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  // 2. 서버 주소 설정 (로컬 PC 9000번 포트)
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9000);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  // 3. 서버에 연결 요청 (3-way handshake)
  connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));

  // 4. 실제 전송할 채팅 메시지
  const char *msg = "HELLO";

  // 5. 패킷 헤더 생성
  PacketHeader header;
  header.length = htonl(sizeof(PacketHeader) + strlen(msg)); // 길이
  header.type = htons(0x0003);                               // CHAT 패킷
  header.flags = 0;

  // 6. 헤더 전송
  send(sock, &header, sizeof(header), 0);

  // 7. 바디 전송
  send(sock, msg, strlen(msg), 0);

  // 8. 연결 종료
  close(sock);
}

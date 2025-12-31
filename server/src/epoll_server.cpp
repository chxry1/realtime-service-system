#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────
// 패킷 헤더 정의 (클라이언트와 공통 규약)
// ─────────────────────────────────────────────
#pragma pack(push, 1)
struct PacketHeader {
  uint32_t length; // 전체 패킷 길이 (헤더 + 바디)
  uint16_t type;   // 패킷 타입
  uint16_t flags;  // 옵션
};
#pragma pack(pop)

const uint32_t MAX_PACKET_SIZE = 4096;

// 클라이언트별 누적 수신 버퍼
std::unordered_map<int, std::vector<char>> clientBuffers;

// 소켓을 논블로킹 모드로 전환
void setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {

  // 1. 서버 소켓 생성
  int serverSock = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSock < 0) {
    std::cerr << "socket error\n";
    return 1;
  }

  setNonBlocking(serverSock);

  int opt = 1;
  setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // 2. bind 설정
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9000);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(serverSock, (sockaddr *)&addr, sizeof(addr));

  // 3. listen
  listen(serverSock, 128);

  // 4. epoll 인스턴스 생성
  int epfd = epoll_create1(0);

  // 5. 서버 소켓 epoll 등록
  epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = serverSock;
  epoll_ctl(epfd, EPOLL_CTL_ADD, serverSock, &ev);

  std::vector<epoll_event> events(64);

  std::cout << "epoll packet server running on port 9000\n";

  // 6. 이벤트 루프
  while (true) {

    int n = epoll_wait(epfd, events.data(), events.size(), -1);

    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      // 신규 접속
      if (fd == serverSock) {
        int clientSock = accept(serverSock, nullptr, nullptr);
        setNonBlocking(clientSock);

        epoll_event clientEv{};
        clientEv.events = EPOLLIN;
        clientEv.data.fd = clientSock;
        epoll_ctl(epfd, EPOLL_CTL_ADD, clientSock, &clientEv);
      }
      // 기존 클라이언트 처리
      else {
        char buf[1024];
        int len = recv(fd, buf, sizeof(buf), 0);

        if (len <= 0) {
          close(fd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
          clientBuffers.erase(fd);
          continue;
        }

        auto &recvBuffer = clientBuffers[fd];
        recvBuffer.insert(recvBuffer.end(), buf, buf + len);

        // ───────── 패킷 파싱 루프 ─────────
        while (recvBuffer.size() >= sizeof(PacketHeader)) {

          PacketHeader *header =
              reinterpret_cast<PacketHeader *>(recvBuffer.data());

          uint32_t packetLen = ntohl(header->length);
          uint16_t type = ntohs(header->type);

          // 길이 위변조 차단
          if (packetLen > MAX_PACKET_SIZE || packetLen < sizeof(PacketHeader)) {
            std::cerr << "[SECURITY] malformed packet detected\n";
            close(fd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            clientBuffers.erase(fd);
            break;
          }

          // 아직 전체 패킷 안 들어왔으면 대기
          if (recvBuffer.size() < packetLen)
            break;

          // 임시 처리: 받은 패킷 그대로 에코
          send(fd, recvBuffer.data(), packetLen, 0);

          // 처리 완료 패킷 제거
          recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + packetLen);
        }
      }
    }
  }
}

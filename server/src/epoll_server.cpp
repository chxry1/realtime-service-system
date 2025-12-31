#include <fcntl.h> // fcntl() : 소켓을 논블로킹 모드로 변경
#include <sys/epoll.h> // epoll_create, epoll_ctl, epoll_wait : 이벤트 기반 서버의 핵심 엔진
#include <unordered_map> // 클라이언트별 recvBuffer, 상태 관리용
#include <vector>        // epoll 이벤트 목록을 담는 컨테이너

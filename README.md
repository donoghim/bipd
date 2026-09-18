# BIP daemon test project

이 저장소는 C로 작성된 BIP(Bearer Independent Protocol) 테스트용 프로그램과
실행 보조 스크립트를 담고 있습니다. 기본 빌드 대상은 `bipd_tx1.c`이며,
빌드 결과는 `bipd.exe`라는 이름으로 생성됩니다.

## 먼저 확인할 점: `bipd.exe`는 Linux에서 실행되는가?

가능합니다. 파일 이름의 `.exe` 확장자는 Windows 실행 파일이라는 뜻이
아닙니다. `0_build_bip.sh`가 다음처럼 Linux의 `gcc`로 출력 파일 이름만
`bipd.exe`로 지정하고 있습니다.

```bash
gcc bipd_tx1.c -o bipd.exe -ljson-c
```

따라서 실제 실행 가능 여부는 확장자가 아니라 파일 형식으로 판단해야 합니다.

```bash
file bipd.exe
ldd bipd.exe
```

이 저장소에서 확인된 `bipd.exe`는 다음 조건을 만족하는 Linux ELF 실행 파일입니다.

- 64비트 x86-64 Linux ELF
- `/lib64/ld-linux-x86-64.so.2` 사용
- `libjson-c.so.4` 필요

즉 `systemd`가 `bipd.exe`를 실행하는 것은 정상입니다. 단, systemd가
`bipd.exe`라는 파일명을 자동으로 찾는 것은 아닙니다. 실제 실행 파일은
`/etc/systemd/system/bipd.service`의 `ExecStart=`에 지정되어 있어야 합니다.

## 구성

현재 저장소의 주요 파일은 다음과 같습니다.

| 파일 | 설명 |
| --- | --- |
| `0_build_bip.sh` | 서버, `bipd`, 클라이언트를 빌드 |
| `1_run_bipd.sh` | `bipd.service`를 load/start/stop/restart/status 등으로 제어 |
| `2_check_bipd_state.sh` | 프로세스와 포트 상태 확인 |
| `3_check_bipd_log.sh` | systemd journal을 실시간으로 확인 |
| `4_check_bipd_log.sh` | systemd journal의 최근 로그 확인 |
| `bipd_tx1.c` | 현재 기본으로 빌드되는 단일 TX 테스트 프로그램 |
| `bipd_tx2.c` | 연속 TX 테스트용 대체 구현 |
| `bipd_tls.c` | 별도 TLS 구현 |
| `bipd.c` | 기본 BIP 구현의 다른 소스 |
| `param.json` | IMSI, MSISDN, 포트 등 실행 설정 |
| `bipd.exe` | `bipd_tx1.c`에서 생성되는 Linux 실행 파일 |
| `bip_server.exe` | BIP 서버 테스트 실행 파일 |
| `bip_client.exe` | BIP 클라이언트 테스트 실행 파일 |

저장소에는 systemd 서비스 파일 자체가 포함되어 있지 않습니다. `1_run_bipd.sh`의
첫 주석에 표시된 것처럼 서비스 파일은 운영체제의
`/etc/systemd/system/bipd.service`에 별도로 설치되어 있어야 합니다.

## 필요한 환경

Ubuntu/Debian 계열을 기준으로 다음 패키지가 필요합니다.

```bash
sudo apt update
sudo apt install -y build-essential libjson-c-dev net-tools
```

`ip tuntap` 명령이 없는 시스템에서는 다음 패키지도 설치합니다.

```bash
sudo apt install -y iproute2
```

실행 시 다음 권한과 환경이 필요합니다.

- root 권한 또는 네트워크 인터페이스를 생성할 수 있는 권한
- `bip`라는 TAP 인터페이스를 생성할 수 있는 권한
- `123.123.123.123` 주소를 `bip` 인터페이스에 추가할 수 있는 권한
- TCP 포트 `22001`을 사용할 수 있는 상태
- `libjson-c.so.4`가 설치된 상태

## 빌드

프로젝트 디렉터리에서 실행합니다.

```bash
cd /path/to/bipd
chmod +x 0_build_bip.sh 1_run_bipd.sh 2_check_bipd_state.sh \
  3_check_bipd_log.sh 4_check_bipd_log.sh
./0_build_bip.sh
```

현재 스크립트는 다음 세 파일을 생성하거나 갱신합니다.

```text
bip_server.exe  <- bip_server.c
bipd.exe        <- bipd_tx1.c, -ljson-c
bip_client.exe  <- bip_client.c
```

`bipd_tx2.c`를 빌드하려면 `0_build_bip.sh`에서 `bipd_tx1.c` 줄을 주석 처리하고
`bipd_tx2.c` 줄의 주석을 해제한 뒤 다시 빌드합니다. `bipd_tls.c`도 같은 방식으로
선택할 수 있지만, 해당 소스에 필요한 라이브러리와 동작 조건을 별도로 확인해야
합니다.

빌드 직후 실행 파일 형식을 확인하는 것이 좋습니다.

```bash
file bipd.exe
ldd bipd.exe
```

## `param.json` 설정

`bipd_tx1.c`는 `./param.json`을 현재 작업 디렉터리에서 읽습니다. 서비스로 실행할
때도 `WorkingDirectory`를 프로젝트 디렉터리로 지정해야 합니다.

| 키 | 현재 예시 | 설명 |
| --- | ---: | --- |
| `imsi` | `450051033330008` | 전송할 IMSI |
| `imsi_lgu` | `450061033330008` | 보조 IMSI 값 |
| `imsi_skt` | `450051033330008` | 보조 IMSI 값 |
| `msisdn_alphaId` | `abcdefgh` | MSISDN alpha ID |
| `msisdn_bcdLen` | `7` | MSISDN BCD 길이 |
| `msisdn_dialNum` | `821033330008` | MSISDN dial number |
| `accpt_mode` | `2` | `1`: 모든 주소 허용, `2`: 나열된 주소 모드 |
| `port` | `22001` | TCP listen 포트 |
| `finish_mode` | `2` | `1`: IMSI 후 종료, `2`: MSISDN 후 종료 |

주의할 점:

- `imsi`는 소스의 길이 검사를 통과해야 합니다.
- `msisdn_dialNum`은 짝수 길이의 숫자 문자열이어야 합니다.
- `msisdn_bcdLen`은 dial number를 담을 수 있어야 합니다.
- 실행 중인 서비스가 있으면 `param.json` 변경 후 재시작해야 합니다.
- 실제 테스트 환경의 IMSI와 전화번호를 공개 저장소에 커밋하지 않도록 주의합니다.

## systemd 서비스 설치

`1_run_bipd.sh`는 서비스 파일을 생성하지 않습니다. 다음 예시를 실제 경로에 맞게
작성하여 `/etc/systemd/system/bipd.service`로 설치합니다.

```ini
[Unit]
Description=BIP test daemon
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
WorkingDirectory=/opt/bipd
ExecStart=/opt/bipd/bipd.exe
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

예를 들어 저장소를 `/opt/bipd`에 설치했다면 다음처럼 준비합니다.

```bash
sudo install -d /opt/bipd
sudo cp -a . /opt/bipd/
sudo install -m 0755 bipd.exe /opt/bipd/bipd.exe
sudo install -m 0644 param.json /opt/bipd/param.json
sudo install -m 0644 bipd.service /etc/systemd/system/bipd.service
```

그 다음 systemd를 반영하고 서비스를 시작합니다.

```bash
sudo systemctl daemon-reload
sudo systemctl enable bipd
sudo systemctl start bipd
sudo systemctl status bipd
```

`ExecStart`와 `WorkingDirectory`는 반드시 실제 파일 위치와 일치해야 합니다.
특히 `WorkingDirectory`가 빠지면 프로그램이 `./param.json`을 찾지 못할 수 있습니다.

## `1_run_bipd.sh` 사용법

스크립트는 다음 인자를 지원합니다.

```bash
./1_run_bipd.sh load
./1_run_bipd.sh start
./1_run_bipd.sh status
./1_run_bipd.sh restart
./1_run_bipd.sh stop
./1_run_bipd.sh enable
./1_run_bipd.sh disable
```

각 인자는 다음 systemd 명령으로 연결됩니다.

| 인자 | 실제 명령 |
| --- | --- |
| `load` | `sudo systemctl daemon-reload` |
| `start` | `sudo systemctl start bipd` |
| `status` | `sudo systemctl status bipd` |
| `restart` | `sudo systemctl restart bipd` |
| `stop` | `sudo systemctl stop bipd` |
| `enable` | `sudo systemctl enable bipd` |
| `disable` | `sudo systemctl disable bipd` |

따라서 `sudo systemctl start bipd` 자체는 `bipd.exe`를 직접 실행하는 명령이
아닙니다. `bipd`라는 이름의 systemd 유닛을 찾고, 그 유닛의 `ExecStart`에 적힌
프로그램을 실행합니다. 서비스 파일이 없거나 `ExecStart`가 잘못되면 시작되지
않습니다.

## 실행 흐름

기본 `bipd.exe`는 시작할 때 다음 작업을 수행합니다.

1. 기존 `bip` TAP 인터페이스를 내리고 삭제합니다.
2. `bip` TAP 인터페이스를 만들고 활성화합니다.
3. `123.123.123.123` 주소를 `bip` 인터페이스에 추가합니다.
4. `param.json`을 읽고 BIP 설정을 구성합니다.
5. 설정된 TCP 포트(기본값 `22001`)에서 연결을 기다립니다.
6. BIP 요청에 대해 welcome, IMSI, MSISDN, 테스트 데이터 응답을 전송합니다.

프로그램은 `syslog`에 `bipLog`라는 식별자로 로그를 기록하므로 systemd로
실행하면 `journalctl -u bipd`에서 확인할 수 있습니다.

## 상태 및 로그 확인

```bash
./2_check_bipd_state.sh
```

이 스크립트는 `bipd.exe` 프로세스와 포트 `22001` 사용 여부를 확인합니다.
`netstat`가 없는 시스템에서는 `ss`로 직접 확인할 수 있습니다.

```bash
ps -ef | grep -w bipd.exe
sudo ss -lntp | grep ':22001'
```

실시간 로그:

```bash
./3_check_bipd_log.sh
# 또는
sudo journalctl -u bipd -f
```

최근 로그:

```bash
./4_check_bipd_log.sh
# 또는
sudo journalctl -u bipd -n 100 --no-pager
```

## 문제 해결

### `Unit bipd.service could not be found`

서비스 파일이 설치되지 않았거나 파일명이 다릅니다.

```bash
ls -l /etc/systemd/system/bipd.service
sudo systemctl daemon-reload
sudo systemctl status bipd
```

### `Failed to execute ... bipd.exe: Permission denied`

실행 권한과 파일 형식을 확인합니다.

```bash
chmod +x /opt/bipd/bipd.exe
file /opt/bipd/bipd.exe
```

### `param.json`을 열 수 없음

서비스의 `WorkingDirectory`가 `param.json`이 있는 디렉터리인지 확인합니다.
또한 `ExecStart`를 절대 경로로 사용합니다.

```ini
WorkingDirectory=/opt/bipd
ExecStart=/opt/bipd/bipd.exe
```

### TAP 인터페이스 또는 IP 주소 생성 실패

권한, `ip` 명령 설치 여부, 기존 인터페이스 상태를 확인합니다.

```bash
ip link show bip
ip addr show bip
which ip
sudo journalctl -u bipd -n 100 --no-pager
```

### 포트가 이미 사용 중임

```bash
sudo ss -lntp | grep ':22001'
```

기존 `bipd`를 중지하거나 `param.json`의 포트를 변경한 뒤 서비스를 재시작합니다.

## 주의사항

- 이 코드는 테스트 목적의 프로그램입니다. 운영 서비스에 적용하기 전에 코드와
  네트워크 보안 설정을 별도로 검토해야 합니다.
- 소스는 `sudo ip ...`, `ip tuntap ...`를 호출하므로 실행 계정과 sudo 정책에
  따라 동작이 달라질 수 있습니다.
- systemd 서비스에서는 실행 파일과 설정 파일의 절대 경로, 작업 디렉터리, 권한을
  명시적으로 관리하는 것이 중요합니다.
- `a.out`, 로그 파일, 컴파일된 실행 파일은 환경에 따라 달라질 수 있으므로 배포
  시에는 필요한 파일만 선별하는 것이 좋습니다.
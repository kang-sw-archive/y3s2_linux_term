
# 순서대로 할 일

## 지침
- 가능한 rendering thread는 최적화. 무조건 렌더링 작업이 가장 길어지게 돼있음.

## TODO - Core System
1. [x] Drawing library 선택
   1. [x] Cairo 활용
2. [x] Resource system 정의
3. [ ] Rendering Interface 정의 
   1. [x] Rendering Loop
   2. [x] Queueing Draw Calls 
      1. [x] Image Draw
      2. [x] Font Draw
      3. [ ] Rectangle Draw
   3. [x] Translation Algorithm ... 
         - Transform에서 Position은, 카메라의 위치를 빼고, 회전시킨 뒤, 스케일하여 구함.
         - 렌더링 시점에서(cairo), 먼저 Transform의 Scale Factor를 이용해 종횡 이미지 스케일
         - Rotate Factor로 이미지 회전시킨 후
         - 위에서 구한 Position에 그린다 ... 단, 중점이 되어야 하므로 이를 고려.
4. [ ] Global Function Timer 구현.
   * 단, 게임 스레드에서 실행되게끔.
5. [x] Rendering Implementation
   1. [x] 이미지 렌더
   2. [x] 폰트 렌더
   3. [ ] 랙탱글 렌더
6. [ ] Delegate 구현
   * Game Object는 Handle을 통해서 
7. [ ] Game Object 구현
   1. [ ] Update 반응
   2. [ ] Event 반응
      1. [ ] OnClicked
      2. [ ] OnCollide
   3. [ ] Draw 반응
8. [ ] Game Round 구현
   * Game Round부터 상속 가능... 게임의 성질 정의
9.  [ ] Update 로직 구현
   1.  [ ] Program Instance의 Update
       1.  [ ] Play Waves
       2.  [ ] GameState.Update()
   2.  [ ] Game State의 Update
       1.  [ ] GameRound.HandleInput();
       2.  [ ] GameRound.Update();
       3.  [ ] Update Active Game Objects
       4.  [ ] Draw Active Game Objects
10. [ ] Collision 로직
11. [ ] 입력 처리
12. [ ] 사운드 처리


## TODO - Term project
1. [ ] 터치 입력 Event로 Wrapping하기
2. [ ] Widget 클래스 구현하기 
      - OnTouch() 반응만 갖는다.
      - 터치 입력 받는동안 Resource가 눌린 상태의 버튼 모양으로 변경
      - 중점과 선택 영역 갖음
3. [ ] 사운드 함수성 project-sound 구현
4. [ ] Fruit 클래스 구현하기
   1. [ ] Instantiation.
   2. [ ] 트랜스폼에 따른 visualization
   3. [ ] 업데이트 루틴에 따른 위치 업데이트
   4. [ ] 충돌 반응 (터치에 대해)
5. [ ] 사용자 입력 구현하기
   1. [ ] 사용자 손 위치 추적
   2. [ ] Fruit 클래스에 닿으면 터뜨리는 루틴 추가하기
6. [ ] 게임 오버 화면
   1. [ ] 누르면 랭킹에 이름 등록하는 버튼
   2. [ ] 취소도 있음
7. [ ] 랭킹 
   1. [ ] 파일에 정형 바이너리로 랭킹 정보 저장
   2. [ ] 1~10위 열람 가능
   3. [ ] 초기화 기능도 제공

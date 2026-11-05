// ============================================================================
// ESP32 스마트글래스 탐지기 — 휴대용 하우징 (파라메트릭, 3D 프린터용)
//
// 부품: Seeed XIAO ESP32S3 + 0.96" I2C OLED(128x64) + LiPo 배터리 + 외부 버튼
// 구조: 바닥 트레이(base) + 뚜껑(lid), 네 모서리 M2 셀프탭 나사 결합
//   - 배터리(맨 아래) → XIAO(가운데) → OLED(뚜껑에 장착)로 Z축 적층
//   - 부품은 XY 중앙, 나사 기둥은 네 모서리 → 서로 간섭 없음(치수 자동 계산)
//
// 렌더링(상단 part 변수):
//   "base"    바닥만 (프린트)
//   "lid"     뚜껑만 (프린트 자세: 화면 창이 바닥으로)
//   "both"    둘 다 나란히
//   "preview" 조립 모습
//   "ghost"   부품 간섭 확인용 (부품 외곽을 반투명으로 표시)
//
// 치수가 다른 부품을 쓰면 아래 파라미터 숫자만 바꾸면 전체가 다시 맞춰집니다. (mm)
// ============================================================================

part = "both";
$fn = 56;

// ---------------- 벽 / 공차 ----------------
wall     = 2.0;    // 바깥벽
floor_th = 1.6;    // 바닥판
lid_th   = 1.8;    // 뚜껑판
fit      = 0.3;    // 끼움 공차 (빡빡하면 ↑)
clr      = 0.8;    // 부품-기둥 사이 최소 간격
corner_r = 3.5;    // 바깥 모서리 라운드

// ---------------- 나사 (M2 셀프탭) ----------------
boss_d       = 5.2;  // 나사 기둥 바깥지름
screw_pilot  = 1.6;  // base 파일럿(셀프탭)
screw_free   = 2.4;  // lid 통과구멍
screw_head_d = 4.0;  // 납작머리 자리파기
screw_head_h = 1.8;
lip_wall     = 1.2;  // 뚜껑 턱 두께

// ---------------- XIAO ESP32S3 ----------------
xiao_l     = 21.0;   // USB-C~반대편 길이(X)
xiao_w     = 17.5;   // 폭(Y)
xiao_h     = 1.2;
xiao_usb_w = 9.2;    // USB-C 커넥터 폭
xiao_usb_h = 3.4;    // 높이
xiao_usb_z = 1.1;    // PCB 윗면에서 커넥터 바닥까지

// ---------------- OLED 0.96" 모듈 ----------------
oled_l       = 27.3; // X
oled_w       = 27.5; // Y
oled_pcb_th  = 1.3;
oled_win_w   = 25.0; // 화면 창 가로
oled_win_h   = 14.5; // 화면 창 세로
oled_win_oy  = 0.0;  // 창 중심의 Y 오프셋(모듈 중심 기준, 핀 반대로 +)
oled_hole_dx = 23.2; // 장착 구멍 간격
oled_hole_dy = 23.2;
oled_standoff_h = 3.5;

// ---------------- 배터리 (503040 500mAh 예시) ----------------
bat_l = 40.0;  // X
bat_w = 30.0;  // Y
bat_h = 5.5;   // Z

// ---------------- 외부 버튼 / 부저 ----------------
btn_hole_d = 3.8;    // 뚜껑 버튼 플런저 구멍
btn_seat_d = 6.6;    // 뚜껑 안쪽 6mm 택트스위치 자리
btn_seat_h = 1.2;
buzzer_d   = 12.0;   // 소리 구멍 패턴 크기
buzzer_hole = 1.8;

// ============================================================================
// 자동 치수 계산 — 부품은 중앙, 기둥은 모서리, 간섭 0 보장
// ============================================================================
boss_inset = boss_d/2 + lip_wall + clr + 0.5;      // 벽에서 기둥 중심까지
pad        = clr + boss_inset + boss_d/2;           // 부품 끝에서 벽까지 필요한 여유

max_cx = max(bat_l, oled_l);
max_cy = max(bat_w, oled_w);
inner_x = max_cx + 2*pad;
inner_y = max_cy + 2*pad;

// Z 적층 높이
stack_gap = 1.2;                 // 배터리 위~XIAO 사이
head_room = 4.0;                 // XIAO 부품/커넥터 여유
inner_z = floor_th + bat_h + stack_gap + xiao_h + head_room
          + oled_standoff_h + oled_pcb_th + 1.2;   // 마지막은 유리~뚜껑 여유
base_h = inner_z;                // 바닥 트레이 전체 높이(뚜껑 제외)
lip_h  = 4.0;

out_x = inner_x + 2*wall;
out_y = inner_y + 2*wall;

// 기둥 위치(모서리)
bx = inner_x/2 - boss_inset;
by = inner_y/2 - boss_inset;
boss_pos = [[bx,by],[bx,-by],[-bx,by],[-bx,-by]];

// 부품 중심
bat_c   = [0, 0];
oled_c  = [0, 0];
xiao_c  = [ inner_x/2 - xiao_l/2 - clr, 0 ];        // USB-C가 +X 벽에 닿도록
usb_z   = floor_th + bat_h + stack_gap + xiao_h + xiao_usb_z + xiao_usb_h/2;
btn_c   = [ -inner_x/2 + 8, 0 ];                    // 창 왼쪽 여백
buzz_c  = [  inner_x/2 - 9, -inner_y/2 + 9 ];       // 창 오른쪽 아래 모서리

echo(str("외형(mm):  ", out_x, " x ", out_y, " x ", base_h + lid_th));
echo(str("내부여유 X: 기둥안쪽까지=", inner_x/2 - boss_inset - boss_d/2,
         "  배터리끝=", max_cx/2, "  간격=", inner_x/2 - boss_inset - boss_d/2 - max_cx/2));

// ============================================================================
module rrect(x,y,r){ hull() for(sx=[-1,1],sy=[-1,1]) translate([sx*(x/2-r),sy*(y/2-r)]) circle(r); }
module rbox(x,y,z,r){ linear_extrude(z) rrect(x,y,r); }

// 턱: 네 변 세그먼트로만 (모서리 기둥은 피함)
module lid_lip(){
  seg = 0.70;  // 각 변에서 길이 비율
  w = inner_x - 2*fit;  d = inner_y - 2*fit;
  translate([0,0,-lip_h]) {
    // X 방향 두 변
    for(sy=[-1,1]) translate([0, sy*(d/2 - lip_wall/2),0])
      cube([w*seg, lip_wall, lip_h], center=true);
    // Y 방향 두 변
    for(sx=[-1,1]) translate([sx*(w/2 - lip_wall/2), 0,0])
      cube([lip_wall, d*seg, lip_h], center=true);
  }
}

module bosses(h, hole){
  for(p=boss_pos) translate([p[0],p[1],0]) difference(){
    cylinder(d=boss_d, h=h);
    if(hole>0) translate([0,0,-0.1]) cylinder(d=hole, h=h+0.2);
  }
}

// ============================================================================
// BASE
// ============================================================================
module base(){
  difference(){
    union(){
      difference(){
        rbox(out_x,out_y,base_h,corner_r);
        translate([0,0,floor_th]) rbox(inner_x,inner_y,base_h,max(corner_r-wall,0.6));
      }
      // 모서리 나사 기둥(바닥~윗면)
      translate([0,0,floor_th]) bosses(base_h-floor_th, screw_pilot);
      // 배터리 칸: 얇은 벽으로 둘러 고정
      translate([0,0,floor_th]) difference(){
        rbox(bat_l+2*fit+2.4, bat_w+2*fit+2.4, bat_h+1.5, 1.2);
        translate([0,0,-0.1]) rbox(bat_l+2*fit, bat_w+2*fit, bat_h+2, 0.8);
      }
      // XIAO 받침 레일(배터리 위 높이까지)
      for(sy=[-1,1]) translate([xiao_c[0], sy*(xiao_w/2-1.0), floor_th])
        cube([xiao_l, 2.0, bat_h+stack_gap], center=true);
    }
    // USB-C 개구부(+X 벽)
    translate([out_x/2 - wall/2, xiao_c[1], usb_z])
      cube([wall*3, xiao_usb_w+2*fit, xiao_usb_h+2*fit], center=true);
  }
}

// ============================================================================
// LID
// ============================================================================
module buzzer(){
  cylinder(d=buzzer_hole, h=lid_th+1, center=true);
  for(a=[0:60:300]) rotate([0,0,a]) translate([buzzer_d/3,0,0])
    cylinder(d=buzzer_hole, h=lid_th+1, center=true);
}

module lid(){
  difference(){
    union(){
      rbox(out_x,out_y,lid_th,corner_r);
      lid_lip();
      // OLED 장착 스탠드오프 4개(아래로)
      for(sx=[-1,1],sy=[-1,1])
        translate([oled_c[0]+sx*oled_hole_dx/2, oled_c[1]+sy*oled_hole_dy/2, -oled_standoff_h])
          difference(){ cylinder(d=4.6,h=oled_standoff_h); translate([0,0,-0.1]) cylinder(d=1.6,h=oled_standoff_h+0.2); }
    }
    // 나사: 통과구멍 + 머리 자리파기(윗면에서)
    for(p=boss_pos) translate([p[0],p[1],0]){
      translate([0,0,-lip_h-0.1]) cylinder(d=screw_free, h=lid_th+lip_h+0.2);
      translate([0,0,lid_th-screw_head_h]) cylinder(d=screw_head_d, h=screw_head_h+0.1);
    }
    // OLED 창
    translate([oled_c[0], oled_c[1]+oled_win_oy, -0.1])
      rbox(oled_win_w+2*fit, oled_win_h+2*fit, lid_th+0.2, 1.0);
    // 버튼: 플런저 구멍 + 안쪽 스위치 자리
    translate([btn_c[0],btn_c[1],0]){
      translate([0,0,-0.1]) cylinder(d=btn_hole_d, h=lid_th+0.2);
      translate([0,0,-btn_seat_h]) cylinder(d=btn_seat_d, h=btn_seat_h+0.01);
    }
    // 부저 구멍
    translate([buzz_c[0],buzz_c[1],lid_th/2]) buzzer();
  }
}

// ============================================================================
// 부품 간섭 확인용 유령(ghost)
// ============================================================================
module ghost(){
  base();
  // 배터리(중앙)
  color([1,0,0,0.30]) translate([-bat_l/2,-bat_w/2, floor_th]) cube([bat_l,bat_w,bat_h]);
  // XIAO
  color([0,1,0,0.4]) translate([xiao_c[0]-xiao_l/2, -xiao_w/2, floor_th+bat_h+stack_gap]) cube([xiao_l,xiao_w,xiao_h]);
  // OLED PCB (뚜껑 스탠드오프 위 높이)
  oled_z = base_h - 1.2 - oled_pcb_th;
  color([0,0,1,0.4]) translate([-oled_l/2,-oled_w/2, oled_z]) cube([oled_l,oled_w,oled_pcb_th]);
  // 나사 기둥 표시
  color([1,1,0,0.5]) translate([0,0,floor_th]) bosses(base_h-floor_th, screw_pilot);
}

// ============================================================================
if(part=="base") base();
else if(part=="lid") translate([0,0,lid_th]) rotate([180,0,0]) lid();   // 창이 바닥 → 서포트 없이 인쇄
else if(part=="preview"){ color("SteelBlue") base(); color("Khaki") translate([0,0,base_h+lip_h]) lid(); }
else if(part=="ghost") ghost();
else { translate([0,-out_y/2-3,0]) base(); translate([0,out_y/2+3,lid_th]) rotate([180,0,0]) lid(); }

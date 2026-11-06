// ============================================================================
// XIAO ESP32-S3 Plus 1.14" 화면 일체형 보드용 하우징 (크래들 방식)
//
// 이 보드는 화면·버튼 3개·USB-C가 모두 보드에 붙어 있고, 정확한 위치가
// 공개돼 있지 않습니다. 그래서 "앞면(화면·버튼)을 통째로 열어 둔" 받침 구조로
// 설계했습니다 → 내부 부품 위치를 몰라도 화면은 보이고 버튼은 눌리고 USB는 꽂힙니다.
//
//   [뚜껑/프레임 없음]  화면·버튼 = 열린 윗면으로 노출
//   보드 ── 아래 선반에 안착
//   배터리 ── 보드 밑 칸
//   USB-C ── 한쪽 짧은 벽의 슬롯 (usb_end 로 위치 선택)
//
// 보드 크기는 Seeed 위키 "Board Size 26 x 48 x 10.6mm" 기준.
// 실측 후 숫자만 맞추세요. 단위 mm.
//
// 렌더:  part="tray"(받침, 프린트) | "preview"(보드/배터리 유령 포함)
// ============================================================================

part = "tray";
$fn = 56;

// ---------------- 보드 ----------------
board_l = 48.0;   // 긴 변 (X)
board_w = 26.0;   // 짧은 변 (Y)
board_th = 1.6;   // PCB 두께(선반이 받치는 부분). 전체 10.6은 화면+USB 포함값
pcb_fit = 0.4;    // 보드 XY 끼움 여유

// USB-C 위치: 어느 짧은 벽에 둘지. "x+"=긴 변 +X 끝, "x-"=반대 끝
usb_end = "x+";
usb_w = 10.0;     // 슬롯 폭(케이블 몰딩 여유 포함)
usb_h = 5.0;      // 슬롯 높이
usb_z_off = 0.0;  // 보드 윗면 기준 USB 중심 높이 보정(+면 위로)

// ---------------- 배터리 (보드 밑 칸) ----------------
// 기본값: 602540 (6.0 x 25 x 40mm, ~600-650mAh). 포장 비닐 두께까지 여유 포함.
bat_l = 42.0;     // X (602540 길이 40mm + 여유 2mm)
bat_w = 26.0;     // Y (602540 폭 25mm + 여유 1mm)
bat_h = 7.0;      // Z (602540 두께 6mm + 여유 1mm)

// ---------------- 케이스 ----------------
wall = 2.0;
floor_th = 1.6;
shelf = 1.6;        // 보드를 받치는 선반 폭(안쪽으로)
wall_above = 2.5;   // 보드 윗면보다 벽을 얼마나 더 올릴지(화면 보호 테두리)
corner_r = 3.0;
retain = true;      // USB 반대쪽 끝에 보드 눌림방지 턱
lanyard = true;     // 끈 구멍 러그

// ---------------- 계산 ----------------
cav_l = board_l + 2*pcb_fit;      // 보드 수용 X
cav_w = board_w + 2*pcb_fit;      // 보드 수용 Y
inner_l = cav_l;
inner_w = cav_w;
board_z = floor_th + bat_h + 0.8; // 보드 밑면 높이(배터리 위)
top_z = board_z + board_th + wall_above;  // 벽 높이
out_l = inner_l + 2*wall;
out_w = inner_w + 2*wall;

module rrect(x,y,r){ hull() for(sx=[-1,1],sy=[-1,1]) translate([sx*(x/2-r),sy*(y/2-r)]) circle(r); }
module rbox(x,y,z,r){ linear_extrude(z) rrect(x,y,r); }

module tray(){
  difference(){
    union(){
      // 바깥 몸통
      difference(){
        rbox(out_l, out_w, top_z, corner_r);
        // 전체 안쪽을 파냄(윗면 열림)
        translate([0,0,floor_th]) rbox(inner_l, inner_w, top_z, max(corner_r-wall,0.6));
      }
      // 보드 선반: 안쪽 둘레에서 shelf 만큼 안으로, board_z 높이에 턱
      translate([0,0,board_z-1.2]) difference(){
        rbox(inner_l, inner_w, 1.2, max(corner_r-wall,0.6));
        translate([0,0,-0.1]) rbox(inner_l-2*shelf, inner_w-2*shelf, 1.4, 0.6);
      }
      // 배터리 칸 측벽(선반 아래 공간을 배터리 크기로 가이드)
      translate([0,0,floor_th]) difference(){
        rbox(min(bat_l+2.4,inner_l), min(bat_w+2.4,inner_w), bat_h, 1.2);
        translate([0,0,-0.1]) rbox(bat_l, bat_w, bat_h+0.2, 0.8);
      }
      // 끈 러그
      if(lanyard)
        translate([-out_l/2-2.5, 0, top_z/2]) difference(){
          rotate([90,0,0]) cylinder(d=7, h=4, center=true);
          rotate([90,0,0]) cylinder(d=3, h=5, center=true);
        }
    }
    // USB-C 슬롯
    ux = (usb_end=="x-") ? -1 : 1;
    translate([ux*(out_l/2 - wall/2), 0, board_z + board_th/2 + usb_z_off])
      cube([wall*3, usb_w, usb_h], center=true);
    // 배터리 선 통과(선반에 홈)
    translate([0, 0, board_z-0.6]) rotate([0,0,0])
      translate([ -ux*(inner_l/2 - shelf/2), 0, 0]) cube([shelf+1, 6, 3], center=true);
  }
  // 보드 눌림 방지 턱(USB 반대쪽 짧은 끝, 버튼 없는 쪽). 안쪽으로 살짝 덮음
  if(retain){
    ux = (usb_end=="x-") ? -1 : 1;
    translate([-ux*(inner_l/2 - 1.0), 0, board_z + board_th])
      cube([2.0, inner_w*0.5, 1.2], center=true);
  }
}

module preview(){
  color("DimGray") tray();
  // 배터리
  color([1,0,0,0.35]) translate([-bat_l/2,-bat_w/2,floor_th]) cube([bat_l,bat_w,bat_h]);
  // 보드
  color([0.1,0.3,0.8,0.5]) translate([-board_l/2,-board_w/2,board_z]) cube([board_l,board_w,board_th]);
  // 화면(보드 위, 열린 윗면으로 노출) — 대략 중앙
  color([0.1,0.1,0.1,0.8]) translate([-25.3/2, -14.2/2, board_z+board_th]) cube([25.3,14.2,1.5]);
}

if(part=="preview") preview();
else tray();

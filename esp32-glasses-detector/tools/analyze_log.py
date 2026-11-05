#!/usr/bin/env python3
"""시리얼 OBS 로그를 분석해서 '연결 전/연결 후' 광고 패턴 차이를 보여줍니다.

사용법:
  pio device monitor | tee log.txt      # 기록
  python3 tools/analyze_log.py log.txt  # 분석

안경을 폰과 끊은 상태로 2분, 연결한 상태로 2분 기록한 뒤 구간별로 비교하면
내 안경이 연결되었을 때 광고 간격/connectable 여부가 어떻게 바뀌는지 알 수 있고,
그 값을 detector.cpp의 usageState() 기준값에 반영하면 됩니다.
"""
import statistics
import sys
from collections import defaultdict


def main(path: str, window_s: float) -> None:
    rows = defaultdict(list)  # (id, company) -> [(ms, rssi, connectable, legacy, name)]
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if not line.startswith("OBS,") or line.startswith("OBS,ms"):
                continue
            parts = line.rstrip("\n").split(",", 8)
            if len(parts) < 8:
                continue
            _, ms, tid, _addr, cid, rssi, conn, legacy = parts[:8]
            name = parts[8] if len(parts) > 8 else ""
            rows[(tid, cid)].append((int(ms), int(rssi), conn == "1", legacy == "1", name))

    if not rows:
        print("OBS 줄이 없습니다. config.h에서 LOG_OBSERVATIONS = true 인지 확인하세요.")
        return

    for (tid, cid), obs in sorted(rows.items(), key=lambda kv: kv[1][0][0]):
        names = {o[4] for o in obs if o[4]}
        print(f"\n=== 대상 #{tid}  company={cid}  이름={', '.join(names) or '-'}  패킷 {len(obs)}개")
        start = obs[0][0]
        buckets = defaultdict(list)
        for o in obs:
            buckets[int((o[0] - start) / 1000 // window_s)].append(o)
        print(f"  {'구간':>10} {'패킷':>5} {'간격중앙값':>10} {'RSSI평균':>8} {'connectable':>11} {'legacy':>6}")
        for b in sorted(buckets):
            seg = buckets[b]
            # 같은 광고의 Scan Response(15ms 미만 간격)는 제외
            gaps = [y[0] - x[0] for x, y in zip(seg, seg[1:]) if y[0] - x[0] >= 15]
            gap = f"{statistics.median(gaps):.0f}ms" if gaps else "-"
            rssi = statistics.mean(o[1] for o in seg)
            conn = sum(o[2] for o in seg) / len(seg)
            legacy = sum(o[3] for o in seg) / len(seg)
            t0 = b * window_s
            print(f"  {t0:>6.0f}-{t0 + window_s:<4.0f}s {len(seg):>5} {gap:>10} {rssi:>8.1f} {conn:>10.0%} {legacy:>6.0%}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    main(sys.argv[1], float(sys.argv[2]) if len(sys.argv) > 2 else 30.0)

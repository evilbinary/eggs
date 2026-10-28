#!/bin/bash
# 生成 wayland 协议 C 代码（本仓库只提交生成结果，扫描器不提交）。
#
# 用法：
#   WAYLAND_SCANNER=/path/to/wayland-scanner ./gen.sh [weston源码目录]
#
# wayland-scanner 由 wayland 源码的 src/scanner.c 编出（需要 libexpat）：
#   cc -O2 -o wayland-scanner src/scanner.c src/wayland-util.c \
#      -Isrc -I../libwayland/gen -lexpat
#
# 输出：
#   gen/<base>-client-protocol.h   客户端头
#   gen/<base>-server-protocol.h   服务端头
#   gen/<base>-protocol.c          编组代码（private-code，1.15+ 扫描器）
#
# 协议清单来自 weston 16 的 protocol/meson.build（generated_protocols）。
set -e
cd "$(dirname "$0")"

SCANNER="${WAYLAND_SCANNER:-wayland-scanner}"
WP=.
GEN=gen
WESTON="${1:-}"

mkdir -p "$GEN"

# 协议名 → xml 相对路径
xml_of() {
    name="$1"; kind="$2"; ver="$3"
    case "$kind" in
        stable)   echo "$WP/stable/$name/$name.xml" ;;
        unstable) echo "$WP/unstable/$name/$name-unstable-$ver.xml" ;;
        staging)  echo "$WP/staging/$name/$name-$ver.xml" ;;
        internal) echo "$WESTON/protocol/$name.xml" ;;
    esac
}

# base 名（与 weston 的命名一致）
base_of() {
    name="$1"; kind="$2"; ver="$3"
    case "$kind" in
        stable)   echo "$name" ;;
        unstable) echo "$name-unstable-$ver" ;;
        staging)  echo "$name-$ver" ;;
        internal) echo "$name" ;;
    esac
}

gen_one() {
    name="$1"; kind="$2"; ver="${3:-}"
    xml=$(xml_of "$name" "$kind" "$ver")
    base=$(base_of "$name" "$kind" "$ver")
    if [ ! -f "$xml" ]; then
        echo "MISSING: $xml" >&2
        return 1
    fi
    "$SCANNER" client-header "$xml" "$GEN/$base-client-protocol.h"
    "$SCANNER" server-header "$xml" "$GEN/$base-server-protocol.h"
    "$SCANNER" private-code  "$xml" "$GEN/$base-protocol.c"
    echo "  $base"
}

# staging
for p in alpha-modifier color-management color-representation commit-timing \
         fifo fractional-scale single-pixel-buffer tearing-control xwayland-shell; do
    gen_one "$p" staging v1
done
# unstable
for p in input-method input-timestamps linux-dmabuf linux-explicit-synchronization \
         pointer-constraints relative-pointer tablet text-input xdg-output; do
    case "$p" in
        tablet) gen_one "$p" unstable v2 ;;
        *)      gen_one "$p" unstable v1 ;;
    esac
done
gen_one xdg-shell unstable v6
# stable
for p in presentation-time viewporter xdg-shell; do
    gen_one "$p" stable
done
# weston 内部协议（来自 weston 源码树）
if [ -n "$WESTON" ]; then
    for p in ivi-application ivi-hmi-controller text-cursor-position weston-debug \
             weston-desktop-shell weston-output-capture weston-content-protection \
             weston-test weston-touch-calibration weston-direct-display; do
        gen_one "$p" internal
    done
fi

echo "done: $(ls "$GEN" | wc -l | tr -d ' ') files in $GEN"

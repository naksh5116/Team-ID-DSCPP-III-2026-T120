#!/bin/sh
# Checks for VitalRoute Web (bin/vitalroute_web), driven with curl.
#
#   sh tests/web_check.sh
#
# Starts the server with --local-only on a free port and a temporary copy of
# data/, so data/records is never touched and no firewall prompt appears.
# Prints one [PASS]/[FAIL] line per check, like the other suites.
cd "$(dirname "$0")/.." || exit 1
make -s web || { echo "[FAIL] build"; exit 1; }

PORT=${WEB_CHECK_PORT:-18080}
BASE="http://127.0.0.1:$PORT"
work=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/vr_web_$$")
mkdir -p "$work"
cp -r data "$work/data"
rm -f "$work/data/records/"*.dat

pass=0
fail=0
check() {   # $1 = id and description, $2 = 0 for pass
    if [ "$2" -eq 0 ]; then
        echo "[PASS] $1"
        pass=$((pass + 1))
    else
        echo "[FAIL] $1"
        fail=$((fail + 1))
    fi
}

./bin/vitalroute_web --port "$PORT" --local-only --web web "$work/data" > "$work/server.log" 2>&1 &
server=$!
cleanup() {
    kill "$server" 2>/dev/null
    [ -n "$sse1" ] && kill "$sse1" 2>/dev/null
    [ -n "$sse2" ] && kill "$sse2" 2>/dev/null
    rm -rf "$work"
}
trap cleanup EXIT

# Wait until the server answers (at most 5 s).
i=0
until curl -s -o /dev/null "$BASE/api/ping" || [ $i -ge 50 ]; do
    sleep 0.1
    i=$((i + 1))
done

echo "-- W1 server core --"
grep -q "On this computer:     http://localhost:$PORT/" "$work/server.log"
check "WB1  Starts and prints the local link" $?

body=$(curl -s "$BASE/api/ping")
echo "$body" | grep -q '"ok":true' && echo "$body" | grep -qF '"links":[]'
check "WB2  GET /api/ping answers JSON (no network links with --local-only)" $?

code=$(curl -s -o "$work/index.out" -w '%{http_code} %{content_type}' "$BASE/")
[ "$code" = "200 text/html; charset=utf-8" ] && grep -q "Open a role window" "$work/index.out"
check "WB3  GET / serves web/index.html as HTML" $?

[ "$(curl -s -o /dev/null -w '%{http_code}' "$BASE/no-such-page.html")" = "404" ]
check "WB4  A missing page is 404" $?

a=$(curl -s --path-as-is -o /dev/null -w '%{http_code}' "$BASE/../data/doctors.txt")
b=$(curl -s --path-as-is -o /dev/null -w '%{http_code}' "$BASE/%2e%2e/data/doctors.txt")
c=$(curl -s --path-as-is -o /dev/null -w '%{http_code}' "$BASE/..%5cdata%5cdoctors.txt")
[ "$a" = "404" ] && [ "$b" = "404" ] && [ "$c" = "404" ]
check "WB5  Files outside web/ cannot be read (../, %2e%2e, backslash)" $?

res=$(curl -s -w ' %{http_code}' "$BASE/api/no-such-call")
echo "$res" | grep -qF '"error":"Not found: no such API call."} 404'
check "WB6  An unknown API call is a 404 with a JSON error" $?

curl -s -v -o /dev/null -o /dev/null "$BASE/api/ping" "$BASE/" 2> "$work/keepalive.log"
grep -qi "Re-using existing connection\|Reusing existing" "$work/keepalive.log"
check "WB7  Keep-alive: two requests share one connection" $?

# 70 KB is too long for a Windows command line, so curl reads it from a file.
{ printf 'X-Big: '; head -c 70000 /dev/zero | tr '\0' 'x'; printf '\n'; } > "$work/big-header.txt"
[ "$(curl -s -o /dev/null -w '%{http_code}' -H @"$work/big-header.txt" "$BASE/api/ping")" = "431" ]
check "WB8  Oversized request headers are refused (431)" $?

# Server-Sent Events: two listeners stay connected.
curl -s -N "$BASE/api/events" > "$work/sse1.out" &
sse1=$!
curl -s -N "$BASE/api/events" > "$work/sse2.out" &
sse2=$!
i=0
until grep -q '"eventClients":2' "$work/ping2.out" 2>/dev/null || [ $i -ge 30 ]; do
    sleep 0.1
    curl -s "$BASE/api/ping" > "$work/ping2.out"
    i=$((i + 1))
done
grep -q '"eventClients":2' "$work/ping2.out" && grep -q '^retry: 2000' "$work/sse1.out" &&
    grep -q '^: connected' "$work/sse2.out"
check "WB9  Event stream: two browsers stay connected and get the stream header" $?

kill "$sse1" "$sse2" 2>/dev/null
wait "$sse1" "$sse2" 2>/dev/null
sse1=""
sse2=""
i=0
until grep -q '"eventClients":0' "$work/ping0.out" 2>/dev/null || [ $i -ge 30 ]; do
    sleep 0.1
    curl -s "$BASE/api/ping" > "$work/ping0.out"
    i=$((i + 1))
done
grep -q '"eventClients":0' "$work/ping0.out"
check "WB10 Closed browsers are noticed and dropped" $?

./bin/vitalroute_web --port "$PORT" --local-only --web web "$work/data" > "$work/second.log" 2>&1
rc=$?
[ $rc -eq 1 ] && grep -q "is already in use" "$work/second.log"
check "WB11 A second server on the same port says the port is in use" $?

lan=$(ipconfig 2>/dev/null | tr -d '\r' | grep -i "IPv4" | head -1 | sed 's/.*: //')
[ -z "$lan" ] && lan=$(hostname -I 2>/dev/null | cut -d' ' -f1)
if [ -n "$lan" ]; then
    [ "$(curl -s -o /dev/null --max-time 3 -w '%{http_code}' "http://$lan:$PORT/api/ping")" = "000" ]
    check "WB12 --local-only refuses the network address ($lan)" $?
else
    echo "[SKIP] WB12 --local-only refuses the network address - no network address found"
fi

# ------------------------------------------------------------ W4-W9 pages --
echo
echo "-- W4-W9 pages --"
ok=0
for p in nurse doctor patient admin; do
    c=$(curl -s -o "$work/$p.out" -w '%{http_code} %{content_type}' "$BASE/$p.html")
    { [ "$c" = "200 text/html; charset=utf-8" ] && grep -q '<script type="module">' "$work/$p.out" &&
      grep -q 'href="app.css"' "$work/$p.out"; } || { echo "     $p.html: $c"; ok=1; }
done
check "WP1  The four role pages are served and load app.css and a module" $ok

ok=0
for m in app.js map.js panels.js; do
    c=$(curl -s -o /dev/null -w '%{content_type}' "$BASE/$m")
    case "$c" in text/javascript*|application/javascript*) ;; *) echo "     $m: $c"; ok=1 ;; esac
done
case "$(curl -s -o /dev/null -w '%{content_type}' "$BASE/app.css")" in text/css*) ;; *) ok=1 ;; esac
check "WP2  The scripts and style sheet are served with the right types" $ok

# Every name a page or module imports from ./x.js is exported by x.js.
ok=0
for p in web/*.html web/panels.js web/map.js; do
    for line in $(tr '\n' ' ' < "$p" | grep -oE 'import \{[^}]*\} from "\./[a-z]+\.js"' | tr -d ' '); do
        mod="web/${line##*./}"; mod="${mod%\"}"
        names=$(echo "$line" | sed -E 's/import\{([^}]*)\}.*/\1/' | tr ',' ' ')
        for name in $names; do
            grep -qE "export (async )?(function|const) $name\b" "$mod" ||
                { echo "     $p imports $name, which $mod does not export"; ok=1; }
        done
    done
done
check "WP3  Every imported name is exported by its module" $ok

# ------------------------------------------------------------- W2 read API --
echo
echo "-- W2 read API --"
get() { curl -s "$BASE$1"; }
first() { grep -o "\"$1\":[^,}]*" | head -1 | sed "s/\"$1\"://; s/\"//g"; }
ids_in() { grep -o '"id":"VR-[0-9-]*"' | sed 's/"id":"//; s/"//' | tr '\n' ' ' | sed 's/ $//'; }
mkdir -p "$work/json"
save() { curl -s "$BASE$1" > "$work/json/$2.json"; cat "$work/json/$2.json"; }

s=$(save /api/state state)
[ "$(echo "$s" | first waitingEmergency)" = "6" ] && [ "$(echo "$s" | first waitingOpd)" = "2" ] &&
    [ "$(echo "$s" | first icuFree)" = "8" ] && [ "$(echo "$s" | first wardFree)" = "22" ] &&
    echo "$s" | grep -q '"next":{"id":"VR-2026-000004"' && echo "$s" | grep -q '"needs":"Needs an ICU bed (8 free)"'
check "WR1  /api/state: waiting 6 + 2, 8 ICU and 22 ward beds free, next VR-2026-000004 needs ICU" $?

q=$(save /api/queues queues)
heap=$(echo "$q" | sed 's/"opd":.*//' | ids_in)
opd=$(echo "$q" | sed 's/.*"opd"://' | ids_in)
[ "$heap" = "VR-2026-000004 VR-2026-000007 VR-2026-000006 VR-2026-000008 VR-2026-000005 VR-2026-000009" ] &&
    [ "$opd" = "VR-2026-000010 VR-2026-000011" ]
check "WR2  /api/queues: heap order (the order the console treats) and OPD FIFO order" $?

p=$(save "/api/patient?id=VR-2026-000010" patient)
dates=$(echo "$p" | sed 's/.*"visits"://' | grep -o '"date":"[0-9-]*"' | sed 's/"date":"//; s/"//')
d1=$(echo "$dates" | sed -n 1p)
d2=$(echo "$dates" | sed -n 2p)
echo "$p" | grep -q '"name":"Ramesh Gupta"' && [ "$(echo "$dates" | wc -l)" -eq 2 ] && [ "$d1" \> "$d2" ] &&
    echo "$p" | grep -q '"hashIndex":{"entries":11'
check "WR3  /api/patient: record, two visits newest first, hash index figures" $?

r=$(curl -s -w ' %{http_code}' "$BASE/api/patient?id=VR-2026-999999")
echo "$r" | grep -qF '{"error":"Not found: no patient with ID VR-2026-999999."} 404'
check "WR4  Unknown patient: 404 with the console's Not found sentence" $?

r=$(curl -s -w ' %{http_code}' "$BASE/api/patient")
echo "$r" | grep -qF '"Enter a patient ID, for example VR-2026-000001."} 400'
check "WR5  Missing ID: 400 saying what to enter" $?

b=$(save "/api/bill?id=VR-2026-000001" bill)
echo "$b" | grep -q '"settled":true' && echo "$b" | grep -q '"totalText":"Rs 11250.00"' &&
    echo "$b" | grep -q '"totalLabel":"Paid"' && [ "$(echo "$b" | grep -o '"category":' | wc -l)" -eq 2 ]
check "WR6  /api/bill: settled history bill, two charge lines, Rs 11250.00 paid" $?

totals=$(save "/api/bills?sort=amount" bills | grep -o '"total":[0-9.]*' | sed 's/"total"://')
sorted=$(echo "$totals" | sort -rn)
[ "$(echo "$totals" | wc -l)" -eq 11 ] && [ "$totals" = "$sorted" ]
check "WR7  /api/bills?sort=amount: all 11 bills, largest first" $?

m=$(save /api/map map)
[ "$(echo "$m" | grep -o '"label":' | wc -l)" -eq "$(grep -c '^N ' data/hospital_graph.txt)" ] &&
    [ "$(echo "$m" | grep -o '"from":' | wc -l)" -eq "$(grep -c '^E ' data/hospital_graph.txt)" ] &&
    [ "$(echo "$m" | grep -o '"code":' | wc -l)" -eq "$(grep -c '^BED ' data/resources.txt)" ] &&
    echo "$m" | grep -q '"floors":\[0,1,2\]' && echo "$m" | grep -q '"label":"Lift A"'
check "WR8  /api/map: every landmark, corridor and bed from the data files; floors 0-2" $?

r=$(curl -s -w ' %{http_code}' "$BASE/api/route?id=VR-2026-000004")
echo "$r" | grep -qF '{"error":"VR-2026-000004 has no bed assigned."} 409'
check "WR9  /api/route for a waiting patient: 409, no bed assigned" $?

t=$(save /api/theatres theatres)
echo "$t" | grep -q '"name":"OT-1"' && echo "$t" | grep -q '"name":"OT-2"' &&
    [ "$(echo "$t" | grep -o '"ring":\[1,2,3,4,5,6,7,8\]' | wc -l)" -eq 2 ]
check "WR10 /api/theatres: two theatres, all 8 slots free in ring order" $?

d=$(save /api/doctors doctors)
[ "$(echo "$d" | grep -o '"seen":' | wc -l)" -eq 6 ] &&
    ! echo "$d" | sed 's/.*"opdDepartments"://; s/"availability".*//' | grep -q 'Emergency Medicine'
check "WR11 /api/doctors: 6 doctors; OPD departments exclude Emergency Medicine" $?

n=$(save "/api/next-doctor?specialty=Cardiology" nextdoctor)
echo "$n" | grep -q '"text":"Next available in Cardiology: Dr. Rohan Mehta - free now"'
check "WR12 /api/next-doctor: the console's sentence" $?

st=$(save /api/staff staff)
[ "$(echo "$st" | grep -o '"role":' | wc -l)" -eq 9 ] &&
    [ "$(echo "$st" | grep -o '"canAuthoriseCharges":true' | wc -l)" -eq 1 ]
check "WR13 /api/staff: 9 staff, one administrator who may authorise charges" $?

a=$(save /api/admissions admissions)
echo "$a" | grep -q '"summary":"10 patient(s) registered ' &&
    echo "$a" | grep -qF '"avl":"AVL index: 4 dates, height 3, rotations LL 0 RR 1 LR 0 RL 0."'
check "WR14 /api/admissions: console defaults, 10 patients, the console's AVL line" $?

r=$(curl -s -w ' %{http_code}' "$BASE/api/admissions?from=yesterday")
echo "$r" | grep -qF '"Dates look like 2026-09-30 or 20260930."} 400'
check "WR15 A bad date: 400 with the console's date hint" $?

ix=$(save /api/indexes indexes)
echo "$ix" | grep -q '"entries":11' && echo "$ix" | grep -q '"height":3' && echo "$ix" | grep -q '"balanced":true'
check "WR16 /api/indexes: 11 hash entries, AVL height 3, balanced" $?

save /api/transfers transfers | grep -q '"pending":\[\]'
check "WR17 /api/transfers: empty desk" $?

r=$(curl -s -X POST -w ' %{http_code}' "$BASE/api/state")
echo "$r" | grep -qF '"Use GET for this call."} 405'
check "WR18 POST to a read call: 405" $?

if command -v powershell.exe > /dev/null 2>&1; then
    bad=""
    for f in "$work"/json/*.json; do
        powershell.exe -NoProfile -Command "Get-Content -Raw '$(cygpath -w "$f" 2>/dev/null || echo "$f")' | ConvertFrom-Json | Out-Null" > /dev/null 2>&1 ||
            bad="$bad $(basename "$f")"
    done
    [ -z "$bad" ]
    check "WR19 Every response above is valid JSON (checked with PowerShell)${bad:+ - bad:$bad}" $?
else
    echo "[SKIP] WR19 JSON validity - PowerShell not available"
fi

# A second server on records the console saved after treating one patient.
cp -r data "$work/data2"
rm -f "$work/data2/records/"*.dat
printf '3\n12\n' | ./bin/vitalroute "$work/data2" > /dev/null
PORT2=$((PORT + 1))
./bin/vitalroute_web --port "$PORT2" --local-only --web web "$work/data2" > "$work/server2.log" 2>&1 &
server2=$!
i=0
until curl -s -o /dev/null "http://127.0.0.1:$PORT2/api/ping" || [ $i -ge 50 ]; do
    sleep 0.1
    i=$((i + 1))
done
rt=$(curl -s "http://127.0.0.1:$PORT2/api/route?id=VR-2026-000004")
echo "$rt" > "$work/json/route.json"
[ "$(echo "$rt" | sed 's/.*"steps":\[//; s/\],"path".*//' | grep -o '","' | wc -l)" -eq 13 ] &&
    echo "$rt" | grep -qF '"summary":"Total distance: 89 m. Estimated walking time: 74 s (1 min 14 s)."' &&
    echo "$rt" | grep -q '"destination":"ICU-1, Bed 1"' && echo "$rt" | grep -q '"floors":\[0,1\]' &&
    echo "$rt" | grep -q '"stepEdges":\[\[\],\[0,1,2\],\[\]' && echo "$rt" | grep -q '"stepVertex":\[0,-1,4,'
check "WR20 /api/route on console-saved records: 14 steps, 89 m / 74 s, floors 0-1, step mapping as the GUI" $?

s2=$(curl -s "http://127.0.0.1:$PORT2/api/state")
grep -q "^Loaded 11 patients" "$work/server2.log" && [ "$(echo "$s2" | first icuFree)" = "7" ] &&
    [ "$(echo "$s2" | first waitingEmergency)" = "5" ]
check "WR21 The web server loads the console's saved records (7 ICU beds free, 5 waiting)" $?
kill "$server2" 2>/dev/null

# ------------------------------------------------------------- W3 actions --
echo
echo "-- W3 actions and live events --"
post() {   # post PATH [curl --data-urlencode args...]; prints body then " CODE"
    p=$1
    shift
    curl -s -X POST -H 'X-VitalRoute: 1' -w ' %{http_code}' "$@" "$BASE$p"
}
msg() { grep -o '"message":"[^"]*"' | head -1 | sed 's/"message":"//; s/"$//'; }

curl -s -N "$BASE/api/events" > "$work/events.out" &
sse1=$!
sleep 0.3

r=$(curl -s -X POST -w ' %{http_code}' --data-urlencode "name=X" "$BASE/api/undo")
r2=$(curl -s -X POST -H 'X-VitalRoute: 1' -H 'Origin: http://evil.example' -w ' %{http_code}' "$BASE/api/undo")
echo "$r" | grep -q ' 403$' && echo "$r2" | grep -q ' 403$'
check "WA1  Request guard: no X-VitalRoute header, or another site's Origin, is refused (403)" $?

r=$(post /api/register-emergency --data-urlencode "name=Asha Rawat" -d age=40 --data-urlencode "complaint=Collapse" \
    -d severity=1 -d insurer=CGHS -d policy=CG-1 -d coverKind=flat -d amount=500)
echo "$r" | msg | grep -qF "Registered VR-2026-000012 (ESI 1). Position in the emergency queue: 3 of 7. Insurance recorded; it is deducted once, when the bill is settled." &&
    echo "$r" | grep -q ' 200$'
check "WA2  Register emergency: the console's sentences, position 3 of 7, insurance recorded" $?

a=$(post /api/register-emergency --data-urlencode "name=" -d age=40 -d complaint=x -d severity=1)
b=$(post /api/register-emergency --data-urlencode "name=Bad Level" -d age=40 -d complaint=x -d severity=9)
echo "$a" | grep -qF '"Enter the patient'"'"'s name."} 400' &&
    echo "$b" | grep -qF '"Registration cancelled: severity 9 is outside the ESI range 1 (most critical) to 5."} 400'
check "WA3  Refusals: missing name; ESI 9 gives the console's Registration cancelled sentence" $?

r=$(post /api/register-opd --data-urlencode "name=Kiran Bisht" -d age=52 --data-urlencode "complaint=Chest discomfort" -d department=Cardiology)
echo "$r" | msg | grep -qxF "Registered VR-2026-000013 for Cardiology. Position in the OPD line: 3."
check "WA4  Register outpatient: joins the back of the OPD line" $?

r=$(post /api/treat-next)
echo "$r" | grep -q '"outcome":"admitted"' &&
    echo "$r" | grep -qF '"Patient: VR-2026-000004  Suresh Kumar, ESI 1 - Cardiac arrest, pulse restored in ambulance"' &&
    echo "$r" | grep -qF '"Bed: ICU-1, Bed 1 (ICU)"' &&
    [ "$(curl -s "$BASE/api/route?id=VR-2026-000004" | grep -o '"distance":[0-9.]*')" = '"distance":89.00' ]
check "WA5  Treat next patient: the console's lines, ICU-1 Bed 1, and the route is now available" $?

before=$(curl -s "$BASE/api/patient?id=VR-2026-000004" | grep -o '"diagnosis":' | wc -l)
r=$(post /api/visit-note -d id=VR-2026-000004 --data-urlencode "doctor=Dr. Meera Iyer" --data-urlencode "diagnosis=Post-arrest review")
after=$(curl -s "$BASE/api/patient?id=VR-2026-000004" | grep -o '"diagnosis":' | wc -l)
echo "$r" | msg | grep -qxF "Visit added in date order to VR-2026-000004." && [ "$after" -eq $((before + 1)) ]
check "WA6  Visit note added in date order" $?

a=$(post /api/theatre/book -d theatre=OT-1 -d patientId=VR-2026-000004 --data-urlencode "procedure=Coronary angiography" -d fee=25000)
b=$(post /api/theatre/release -d theatre=OT-1 -d slot=1)
c=$(post /api/theatre/release -d theatre=OT-1 -d slot=1)
echo "$a" | msg | grep -qxF "Booked OT-1 slot 1, 08:00-09:00 for Suresh Kumar." &&
    echo "$b" | msg | grep -qxF "Released OT-1 slot 1; it is back in the free-slot ring." &&
    echo "$c" | grep -qF '"Theatre: slot 1 in OT-1 is not booked."} 409'
check "WA7  Theatre: book slot 1, release it, releasing again is refused (console wording)" $?

post /api/transfer --data-urlencode "from=General Ward A" -d to=Radiology > /dev/null
post /api/transfer -d urgent=1 -d ambulance=1 -d patientId=VR-2026-000005 -d from=Roadside --data-urlencode "to=Central Emergency Room" > /dev/null
order=$(curl -s "$BASE/api/transfers" | grep -o '"priority":"[A-Za-z]*"' | tr '\n' ' ')
d=$(post /api/transfer/dispatch)
c=$(post /api/transfer/cancel)
e=$(post /api/transfer/dispatch)
[ "$order" = '"priority":"Urgent" "priority":"Routine" ' ] &&
    echo "$d" | msg | grep -q "Ambulance charge added to BL-000005." &&
    echo "$c" | msg | grep -q "^Cancelled TR-0001" && echo "$e" | msg | grep -qxF "No pending requests."
check "WA8  Transfer desk: urgent to the front, dispatch charges the ambulance, cancel routine, then empty" $?

a=$(post /api/charge -d id=VR-2026-000005 -d authoriser=DR-01 -d type=test -d description=X -d price=10)
b=$(post /api/charge -d id=VR-2026-000005 -d authoriser=AD-01 -d type=test --data-urlencode "description=Chest X-ray" -d price=900 -d urgent=1)
echo "$a" | grep -q '"error":"Error: .*} 400' && echo "$b" | msg | grep -q "^Added Rs 1125.00. New subtotal Rs "
check "WA9  Manual charge: a doctor is refused; an administrator adds Rs 1125.00" $?

r=$(post /api/undo)
echo "$r" | msg | grep -qxF "Undone: charge 'Chest X-ray' on BL-000005 (Rs 1125.00)."
check "WA10 Undo: the console's sentence" $?

r=$(post /api/insurance -d id=VR-2026-000005 --data-urlencode "insurer=ICICI Lombard" -d coverKind=percent -d percent=20 -d cap=5000)
echo "$r" | msg | grep -qxF "Insurance recorded; it is deducted once, when the bill is settled." &&
    curl -s "$BASE/api/bill?id=VR-2026-000005" | grep -q '"insurance":"ICICI Lombard'
check "WA11 Insurance recorded on the bill" $?

a=$(post /api/discharge -d id=VR-2026-000005)
b=$(post /api/discharge -d id=VR-2026-000004)
echo "$a" | grep -qF '"VR-2026-000005 is still waiting to be seen."} 409' &&
    echo "$b" | msg | grep -q "^ICU-1, Bed 1 is free again. Bill BL-000004 settled: paid Rs " &&
    [ "$(curl -s "$BASE/api/state" | first icuFree)" = "8" ]
check "WA12 Discharge: refused while waiting; then frees ICU-1 Bed 1 and settles the bill" $?

s=$(post /api/save)
r=$(post /api/reload)
echo "$s" | msg | grep -q "^Saved 13 patients, " && [ -f "$work/data/records/patients.dat" ] &&
    echo "$r" | msg | grep -q "^Loaded 13 patients, "
check "WA13 Save writes data/records; reload loads it back" $?

sleep 0.3
kill "$sse1" 2>/dev/null
wait "$sse1" 2>/dev/null
sse1=""
ev=$(tr -d '\r' < "$work/events.out")
echo "$ev" | grep -q '^event: queueChanged' && echo "$ev" | grep -q '^event: bedsChanged' &&
    echo "$ev" | grep -q '^event: theatresChanged' && echo "$ev" | grep -q '^event: transfersChanged' &&
    echo "$ev" | grep -q '^event: staffChanged' && echo "$ev" | grep -q '^event: billsChanged' &&
    echo "$ev" | grep -q '^event: saved' && echo "$ev" | grep -q '^event: reloaded' &&
    echo "$ev" | grep -A1 '^event: routeAssigned' | grep -q '"patientId":"VR-2026-000004"'
check "WA14 Live events: every change name, routeAssigned with the patient ID, saved, reloaded" $?

versions=$(echo "$ev" | grep -o '"version":[0-9]*' | sed 's/"version"://' | uniq)
[ "$(echo "$versions" | sort -n | tr '\n' ' ')" = "$(echo "$versions" | tr '\n' ' ')" ] && [ "$(echo "$versions" | tail -1)" -gt 10 ]
check "WA15 Every event carries a version that only grows" $?

echo
if [ "$fail" -eq 0 ]; then
    echo "ALL WEB CHECKS PASSED ($pass of $((pass + fail)))"
else
    echo "SOME WEB CHECKS FAILED ($pass of $((pass + fail)) passed)"
    echo "--- server log ---"
    cat "$work/server.log"
fi
[ "$fail" -eq 0 ]

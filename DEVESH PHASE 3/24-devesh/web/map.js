// map.js - the hospital floor plan and the walking route, drawn as SVG.
//
// Everything comes from the server: landmarks, corridors and beds from
// GET /api/map (the HospitalMap and ResourceRouter of the running session),
// the route from GET /api/route (Dijkstra path, Navigator steps, and which
// edges each step covers - src/app/RouteSteps, the same code as the Qt map).
//
// Metres to pixels: the SVG viewBox is in METRES, one box over all floors
// (so a lift is at the same place on every floor), with y flipped because
// y grows north but screen y grows down:  svgX = x - minX + margin,
// svgY = maxY - y + margin. The browser then scales the viewBox to the
// element with preserveAspectRatio "meet" = min(widthPx / widthM,
// heightPx / heightM) - the same rule as the Qt FloorPlanWidget.
import { api, h, live } from "./app.js";

const NS = "http://www.w3.org/2000/svg";
const MARGIN = 6;   // metres around the plan, room for labels
let mapData = null;
let mapLoading = null;

function svg(tag, attrs = {}, text) {
  const el = document.createElementNS(NS, tag);
  for (const [k, v] of Object.entries(attrs)) el.setAttribute(k, v);
  if (text !== undefined) el.textContent = text;
  return el;
}

async function loadMap(force = false) {
  if (force) { mapData = null; mapLoading = null; }
  if (!mapLoading) mapLoading = api.get("/api/map").then(m => (mapData = m));
  return mapLoading;
}

const floorTitle = f => (f === 0 ? "Ground floor" : `Floor ${f}`);
const MAJOR = new Set(["WARD", "ICU", "OT", "ER", "RECEPTION", "LIFT", "STAIRS"]);

// Draws one floor into an <svg>, with the route (if any) highlighted.
function drawFloor(floor, route, selected) {
  const m = mapData;
  const b = m.bounds;
  const W = b.maxX - b.minX + 2 * MARGIN + 14;   // extra on the right for labels
  const H = b.maxY - b.minY + 2 * MARGIN;
  const X = x => x - b.minX + MARGIN;
  const Y = y => b.maxY - y + MARGIN;           // y grows north; SVG y grows down
  const byId = new Map(m.nodes.map(n => [n.id, n]));
  const root = svg("svg", { viewBox: `0 0 ${W} ${H}`, class: "map-svg", preserveAspectRatio: "xMidYMid meet",
                            role: "img", "aria-label": `${floorTitle(floor)} plan` });

  // Corridors (thin) and doors (dashed) on this floor.
  for (const e of m.edges) {
    const a = byId.get(e.from), c = byId.get(e.to);
    if (!a || !c || a.floor !== floor || c.floor !== floor) continue;
    root.append(svg("line", { x1: X(a.x), y1: Y(a.y), x2: X(c.x), y2: Y(c.y), stroke: "var(--corridor)",
      "stroke-width": 1, "vector-effect": "non-scaling-stroke", "stroke-dasharray": e.type === "DOOR" ? "4 3" : "none" }));
  }

  // The route on this floor; the selected step's edges wider.
  const path = route ? route.path : [];
  const selectedEdges = new Set(route && selected >= 0 ? route.stepEdges[selected] : []);
  const onRoute = new Set(path);
  for (let k = 0; k + 1 < path.length; ++k) {
    const a = byId.get(path[k]), c = byId.get(path[k + 1]);
    if (a.floor !== floor || c.floor !== floor) continue;
    root.append(svg("line", { x1: X(a.x), y1: Y(a.y), x2: X(c.x), y2: Y(c.y), stroke: "var(--accent)",
      "stroke-width": selectedEdges.has(k) ? 6 : 3, "stroke-linecap": "round", "vector-effect": "non-scaling-stroke" }));
  }

  // Beds per ward/ICU: numbers and how many are free.
  const beds = new Map();
  for (const bed of m.beds) {
    const s = beds.get(bed.node) || { lo: bed.number, hi: bed.number, free: 0 };
    s.lo = Math.min(s.lo, bed.number); s.hi = Math.max(s.hi, bed.number); if (bed.free) s.free++;
    beds.set(bed.node, s);
  }

  // Landmarks, then labels. Minor landmarks (junctions, corridors) are named
  // only when the route passes them. Labels are placed like the Qt map:
  // major ones first, each at the first of right / left / above / below that
  // overlaps no marker or label already placed; a minor label that fits
  // nowhere is left out rather than drawn over another.
  const placed = [];   // boxes in SVG units: [x0, y0, x1, y1]
  const free = (bx) => placed.every(p => bx[2] <= p[0] || bx[0] >= p[2] || bx[3] <= p[1] || bx[1] >= p[3]);
  const nodes = m.nodes.filter(n => n.floor === floor);
  for (const n of nodes) {
    const major = MAJOR.has(n.type);
    const side = major ? 1.6 : 0.9;
    root.append(svg("rect", { x: X(n.x) - side / 2, y: Y(n.y) - side / 2, width: side, height: side,
      fill: "var(--surface)", stroke: major ? "var(--text)" : "var(--text-muted)", "stroke-width": 1, "vector-effect": "non-scaling-stroke" }));
    placed.push([X(n.x) - side / 2, Y(n.y) - side / 2, X(n.x) + side / 2, Y(n.y) + side / 2]);
  }
  // The route's start, destination and lift/stairs squares (drawn below)
  // take room too.
  const marked = new Set();
  for (let k = 0; k < path.length; ++k) {
    const n = byId.get(path[k]);
    const prev = byId.get(path[k - 1]), next = byId.get(path[k + 1]);
    const changesFloor = (prev && prev.floor !== n.floor) || (next && next.floor !== n.floor);
    if (n.floor === floor && (k === 0 || k === path.length - 1 || changesFloor)) {
      placed.push([X(n.x) - 1.9, Y(n.y) - 1.9, X(n.x) + 1.9, Y(n.y) + 1.9]);
      marked.add(n.id);
    }
    // A straight (horizontal or vertical) route segment: keep labels off it.
    if (next && n.floor === floor && next.floor === floor) {
      const x0 = Math.min(X(n.x), X(next.x)), x1 = Math.max(X(n.x), X(next.x));
      const y0 = Math.min(Y(n.y), Y(next.y)), y1 = Math.max(Y(n.y), Y(next.y));
      if (x1 - x0 < 1 || y1 - y0 < 1) placed.push([x0 - 0.4, y0 - 0.4, x1 + 0.4, y1 + 0.4]);
    }
  }
  const inside = ([x, y], w, h) => x >= 0.5 && y >= 0.5 && x + w <= W - 0.5 && y + h <= H - 3.5;   // clear of the scale bar
  const wanted = nodes.filter(n => MAJOR.has(n.type) || onRoute.has(n.id))
                      .sort((a, c) => MAJOR.has(c.type) - MAJOR.has(a.type));
  for (const n of wanted) {
    const major = MAJOR.has(n.type);
    const side = major ? 1.6 : 0.9;
    const bs = beds.get(n.id);
    const lines = [[n.label, 1.7, major ? "major" : ""]];
    if (bs) lines.push([`Beds ${bs.lo}-${bs.hi}, ${bs.free} free`, 1.4, ""]);
    const w = Math.max(...lines.map(([t, fs]) => t.length * fs * 0.52));   // average glyph width
    const h = lines.reduce((s, [, fs]) => s + fs * 1.15, 0);
    const cx = X(n.x), cy = Y(n.y), gap = (marked.has(n.id) ? 1.9 : side / 2) + 0.5;
    const spots = [[cx + gap, cy - h / 2], [cx - gap - w, cy - h / 2],          // right, left
                   [cx - w / 2, cy - gap - h], [cx - w / 2, cy + gap],          // above, below (centred)
                   [cx - 1, cy - gap - h], [cx - 1, cy + gap],                  // above, below (starting at the node)
                   [cx + 1 - w, cy - gap - h], [cx + 1 - w, cy + gap]];         // above, below (ending at the node)
    let spot = spots.find(([x, y]) => inside([x, y], w, h) && free([x, y, x + w, y + h]));
    if (!spot) { if (!major) continue; spot = spots.find(s => inside(s, w, h)) || spots[0]; }
    const [x, y] = spot;
    placed.push([x, y, x + w, y + h]);
    let base = y;
    for (const [t, fs, cls] of lines) {
      base += fs * 1.0;
      root.append(svg("text", { x, y: base, "font-size": fs, class: cls }, t));
      base += fs * 0.15;
    }
  }

  if (route && path.length) {
    // Lifts and stairs used: a square around the landmark on both floors.
    for (let k = 0; k + 1 < path.length; ++k) {
      const a = byId.get(path[k]), c = byId.get(path[k + 1]);
      if (a.floor === c.floor) continue;
      for (const n of [a, c]) if (n.floor === floor)
        root.append(svg("rect", { x: X(n.x) - 1.8, y: Y(n.y) - 1.8, width: 3.6, height: 3.6, fill: "none",
          stroke: "var(--accent)", "stroke-width": selectedEdges.has(k) ? 3 : 2, "vector-effect": "non-scaling-stroke" }));
    }
    const start = byId.get(path[0]), end = byId.get(path[path.length - 1]);
    if (start.floor === floor) root.append(svg("rect", { x: X(start.x) - 1.4, y: Y(start.y) - 1.4, width: 2.8, height: 2.8,
      fill: "var(--surface)", stroke: "var(--accent)", "stroke-width": 2.5, "vector-effect": "non-scaling-stroke" }));
    if (end.floor === floor) root.append(svg("rect", { x: X(end.x) - 1.4, y: Y(end.y) - 1.4, width: 2.8, height: 2.8, fill: "var(--accent)" }));
    // A turn, "Exit the lift" or "Arrive": a ring at that landmark.
    const v = selected >= 0 ? route.stepVertex[selected] : -1;
    const vn = byId.get(v);
    if (vn && vn.floor === floor) root.append(svg("circle", { cx: X(vn.x), cy: Y(vn.y), r: 2.4, fill: "none",
      stroke: "var(--accent)", "stroke-width": 2, "vector-effect": "non-scaling-stroke" }));
  }

  // 10 m scale bar and north.
  const sy = H - 2;
  root.append(svg("line", { x1: 2, y1: sy, x2: 12, y2: sy, stroke: "var(--text)", "stroke-width": 1, "vector-effect": "non-scaling-stroke" }));
  root.append(svg("text", { x: 13, y: sy + 0.5, "font-size": 1.5 }, "10 m"));
  root.append(svg("text", { x: W - 3, y: 3, "font-size": 1.7, class: "major" }, "N"));
  root.append(svg("line", { x1: W - 2.5, y1: 4, x2: W - 2.5, y2: 7.5, stroke: "var(--text)", "stroke-width": 1, "vector-effect": "non-scaling-stroke" }));
  return root;
}

// A route panel: floor tabs (or, on a wall display, every route floor) and
// the numbered steps. Clicking a step shows its floor and highlights it.
export function routeMap({ vertical = false } = {}) {
  let route = null, selected = -1, floor = 0, wall = false, note = "The route appears here when a bed is assigned.";
  const floorTabs = h("div", { class: "floor-tabs" });
  const plans = h("div");
  const title = h("div", { class: "route-title", text: "Route" });
  const steps = h("ol", { class: "steps" });
  const summary = h("div", { class: "route-summary hint" });
  const element = h("div", { class: `routemap${vertical ? " vertical" : ""}` },
    h("div", {}, floorTabs, plans), h("div", {}, title, steps, summary));

  const render = () => {
    if (!mapData) return;
    const routeFloors = route ? route.floors : [];
    floorTabs.replaceChildren(...mapData.floors.map(f => h("button", {
      type: "button", "aria-selected": String(!wall && f === floor), hidden: wall,
      onclick: () => { floor = f; render(); },
      text: floorTitle(f) + (routeFloors.includes(f) ? " (route)" : ""),
    })));
    const shown = wall ? (routeFloors.length ? routeFloors : [0]) : [floor];
    plans.replaceChildren(...shown.map(f => h("div", {}, wall ? h("h2", { text: floorTitle(f) }) : null, drawFloor(f, route, selected))));
    steps.replaceChildren(...(route ? route.steps : []).map((s, i) => h("li", {
      class: i === selected ? "selected" : null,
      onclick: () => { selected = i; const f = route.stepFloor[i]; if (f >= 0) floor = f; render(); },
      text: `${i + 1}. ${s}`,
    })));
    title.textContent = route ? `Route: ${route.origin} to ${route.destination}` : "Route";
    summary.textContent = route ? route.summary : note;
  };

  loadMap().then(render);
  live.on(["bedsChanged", "reloaded"], () => loadMap(true).then(render));   // free bed counts

  return {
    element,
    setRoute(r, emptyNote) {
      route = r; selected = -1;
      if (emptyNote) note = emptyNote;
      if (r && r.path.length) floor = mapData ? (mapData.nodes.find(n => n.id === r.path[0])?.floor ?? 0) : 0;
      loadMap().then(render);
    },
    setWall(on) { wall = on; render(); },
    get stepCount() { return route ? route.steps.length : 0; },
    selectStep(i) { if (route && i >= 0 && i < route.steps.length) { selected = i; const f = route.stepFloor[i]; if (f >= 0) floor = f; render(); } },
  };
}

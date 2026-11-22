// app.js - shared code for every VitalRoute Web page.
//
//  api      GET/POST to the server (actions carry the X-VitalRoute header)
//  live     the event stream: the same change names as the Qt SessionHub
//           (queueChanged, bedsChanged, routeAssigned, ...). A page says
//           which changes it cares about; nothing polls.
//  h        builds DOM nodes. Text always goes in as text, never as HTML,
//           because names typed on one device appear on every other device.
//  statusBar, table, tabs, esiBadge, message: the page furniture.

// ------------------------------------------------------------------ api --
export const api = {
  async get(path) {
    const res = await fetch(path, { cache: "no-store" });
    const body = await res.json().catch(() => ({ error: "Error: the server sent something unexpected." }));
    if (!res.ok) throw new Error(body.error || `Error: ${res.status}.`);
    return body;
  },
  async post(path, fields = {}) {
    const res = await fetch(path, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded", "X-VitalRoute": "1" },
      body: new URLSearchParams(Object.entries(fields).filter(([, v]) => v !== undefined && v !== null)),
    });
    const body = await res.json().catch(() => ({ error: "Error: the server sent something unexpected." }));
    if (!res.ok) throw new Error(body.error || `Error: ${res.status}.`);
    return body;
  },
};

// ----------------------------------------------------------------- live --
const EVENTS = ["queueChanged", "patientsChanged", "bedsChanged", "billsChanged", "theatresChanged",
                "transfersChanged", "staffChanged", "routeAssigned", "saved", "reloaded"];
const handlers = new Map();            // event name -> Set of functions
const pending = new Set();             // refreshes to run on the next frame
let version = 0;
let connectionListener = () => {};

function schedule(fn, data) {
  // Several events often arrive together (a treatment sends five): run each
  // refresh once per burst, not once per event. A timer, not
  // requestAnimationFrame: browsers pause animation frames in background
  // tabs, and a role window that is not in front must still stay current.
  if (data !== undefined) { fn(data); return; }
  if (pending.size === 0) setTimeout(() => { const fns = [...pending]; pending.clear(); fns.forEach(f => f()); }, 30);
  pending.add(fn);
}

export const live = {
  // Run `fn` whenever any of the named changes happens (and after a reconnect).
  on(names, fn) {
    for (const n of [].concat(names)) {
      if (!handlers.has(n)) handlers.set(n, new Set());
      handlers.get(n).add(fn);
    }
    return fn;
  },
  // routeAssigned with its patient ID.
  onRoute(fn) {
    if (!handlers.has("routeAssigned:data")) handlers.set("routeAssigned:data", new Set());
    handlers.get("routeAssigned:data").add(fn);
  },
  refreshAll() {
    const all = new Set();
    for (const [name, set] of handlers) if (!name.includes(":")) set.forEach(f => all.add(f));
    all.forEach(f => schedule(f));
  },
  onConnection(fn) { connectionListener = fn; },
  start() {
    let lostConnection = false;
    const source = new EventSource("/api/events");
    source.onopen = () => {
      connectionListener(true);
      if (lostConnection) live.refreshAll();   // changes may have happened meanwhile
      lostConnection = false;
    };
    source.onerror = () => { lostConnection = true; connectionListener(false); };
    for (const name of EVENTS) {
      source.addEventListener(name, (e) => {
        let data = {};
        try { data = JSON.parse(e.data); } catch { /* keep {} */ }
        if (data.version && version && data.version > version + 1) { version = data.version; live.refreshAll(); return; }
        if (data.version) version = Math.max(version, data.version);
        if (name === "routeAssigned") (handlers.get("routeAssigned:data") || []).forEach(f => f(data));
        (handlers.get(name) || []).forEach(f => schedule(f));
      });
    }
  },
};

// ------------------------------------------------------------------ dom --
export function h(tag, attrs = {}, ...children) {
  const el = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs || {})) {
    if (v === undefined || v === null || v === false) continue;
    if (k === "class") el.className = v;
    else if (k.startsWith("on") && typeof v === "function") el.addEventListener(k.slice(2), v);
    else if (k === "text") el.textContent = v;
    else el.setAttribute(k, v === true ? "" : v);
  }
  for (const c of children.flat(Infinity)) {   // panels pass rows as nested arrays
    if (c === undefined || c === null || c === false) continue;
    el.append(c instanceof Node ? c : document.createTextNode(String(c)));
  }
  return el;
}

export function esiBadge(level, name) {
  if (!(level >= 1 && level <= 5)) return document.createTextNode(name || "");
  return h("span", {}, h("span", { class: `esi l${level}`, text: String(level), title: name }), name || "");
}

// A table from rows of plain values. columns: [{key, label, mono, right, render(row)}].
export function table(columns, rows, { keyOf, selected, onSelect, empty = "Nothing to show." } = {}) {
  const body = rows.length === 0
    ? [h("tr", {}, h("td", { class: "empty", colspan: columns.length, text: empty }))]
    : rows.map(row => {
        const key = keyOf ? keyOf(row) : undefined;
        return h("tr", {
          class: key !== undefined && key === selected ? "selected" : null,
          onclick: onSelect ? () => onSelect(row) : null,
        }, columns.map(c => h("td", { class: [c.mono ? "mono" : "", c.right ? "right" : "", c.wrap ? "wrap" : ""].join(" ").trim() || null },
          c.render ? c.render(row) : (row[c.key] ?? ""))));
      });
  return h("table", {}, h("thead", {}, h("tr", {}, columns.map(c => h("th", { text: c.label })))), h("tbody", {}, body));
}

// Tabs: [{label, build()}]. Returns {element, select(i)}.
export function tabs(items, onChange) {
  const bar = h("div", { class: "tabs", role: "tablist" });
  const panels = h("div");
  const built = [];
  let current = -1;
  const select = (i) => {
    current = i;
    [...bar.children].forEach((b, j) => b.setAttribute("aria-selected", String(j === i)));
    if (!built[i]) { built[i] = items[i].build(); panels.append(built[i]); }
    built.forEach((p, j) => { if (p) p.hidden = j !== i; });
    if (onChange) onChange(i);
  };
  items.forEach((it, i) => bar.append(h("button", { type: "button", role: "tab", onclick: () => select(i), text: it.label })));
  select(0);
  return { element: h("div", {}, bar, panels), select, get current() { return current; } };
}

// ----------------------------------------------------------- status bar --
let messageEl = null;
export function message(text, isError = false) {
  if (!messageEl) return;
  messageEl.textContent = text || "";
  messageEl.title = text || "";
  messageEl.classList.toggle("error", !!isError);
}
export function error(err) { message(err instanceof Error ? err.message : String(err), true); }

// Runs an action; shows its message, or the server's refusal, in the status bar.
export async function attempt(fn) {
  message("");
  try {
    const result = await fn();
    if (result && result.message) message(result.message);
    return result;
  } catch (e) {
    error(e);
    return null;
  }
}

export function statusBar(role) {
  messageEl = h("span", { class: "message" });
  const f = {
    emergency: h("span"), opd: h("span"), next: h("span"), icu: h("span"), ward: h("span"),
    role: h("span", { text: `Role: ${role}` }), link: h("span", { text: "Connecting" }),
  };
  const bar = h("footer", { class: "statusbar" }, messageEl, f.emergency, f.opd, f.next, f.icu, f.ward, f.role, f.link);
  const refresh = async () => {
    try {
      const s = await api.get("/api/state");
      f.emergency.textContent = `Emergency waiting: ${s.waitingEmergency}`;
      f.opd.textContent = `OPD waiting: ${s.waitingOpd}`;
      f.next.replaceChildren("Next: ", s.next ? h("span", { class: "mono", text: s.next.id }) : "nobody waiting",
                             s.next && s.next.kind === "opd" ? " (OPD)" : "");
      f.icu.textContent = `ICU beds free: ${s.icuFree}`;
      f.ward.textContent = `Ward beds free: ${s.wardFree}`;
    } catch (e) { /* the connection field says why */ }
  };
  live.on(["queueChanged", "bedsChanged", "patientsChanged", "reloaded"], refresh);
  live.onConnection(ok => { f.link.textContent = ok ? "Live" : "Reconnecting"; if (ok) refresh(); });
  refresh();
  return bar;
}

// --------------------------------------------------------------- page frame --
// Top bar with the role name and links to the other roles, plus the status bar.
export function page(role, content) {
  document.title = `VitalRoute - ${role}`;
  const roles = [["Doctor", "doctor.html"], ["Nurse", "nurse.html"], ["Patient", "patient.html"], ["Super Admin", "admin.html"]];
  const top = h("header", { class: "topbar" },
    h("span", { class: "title", text: `VitalRoute - ${role}` }),
    h("span", { class: "spacer" }),
    h("span", { class: "hint", text: "Switch role:" }),
    roles.filter(([r]) => r !== role).map(([r, href]) => h("a", { href, text: r })),
    h("a", { href: "index.html", text: "Launcher" }));
  document.body.replaceChildren(top, h("main", {}, content), statusBar(role));
  // Ctrl+S saves (except on the read-only Patient page).
  if (role !== "Patient") {
    document.addEventListener("keydown", (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "s") { e.preventDefault(); attempt(() => api.post("/api/save")); }
    });
  }
  live.start();
}

// ----------------------------------------------------------------- small --
export function field(form, name) { return (form.elements[name]?.value ?? "").trim(); }
export function today() { const d = new Date(); return `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, "0")}-${String(d.getDate()).padStart(2, "0")}`; }
export function option(value, label) { return h("option", { value, text: label ?? value }); }

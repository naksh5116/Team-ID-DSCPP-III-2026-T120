// panels.js - the screen panels, shared by the role pages (the Super Admin
// page reuses the Nurse and Doctor panels, as in the Qt GUI). Each panel
// fetches what it shows from the API and refreshes itself on the live
// events it cares about.
import { api, attempt, esiBadge, field, h, live, message, option, table, tabs, today } from "./app.js";
import { routeMap } from "./map.js";

const ESI = [
  [1, "Resuscitation", "Needs an immediate life-saving intervention"],
  [2, "Emergent", "High-risk, confused or lethargic, or in severe pain or distress"],
  [3, "Urgent", "Stable; expected to need two or more resources"],
  [4, "Less urgent", "Stable; expected to need one resource"],
  [5, "Non-urgent", "Stable; expected to need no resources"],
];

const section = (title, hint, ...content) =>
  h("div", { class: "pad stack" }, h("div", {}, h("h2", { text: title }), hint ? h("div", { class: "hint", text: hint }) : null), ...content);
const wrap = (node, tall) => h("div", { class: `table-wrap${tall ? " tall" : ""}` }, node);
// A form row: the label is tied to its control (for/id), so screen readers
// and a click on the label both reach the field.
let nextId = 0;
const row = (label, control) => {
  if (control instanceof HTMLElement && control.matches("input, select, textarea")) {
    if (!control.id) control.id = `f${++nextId}`;
    return [h("label", { for: control.id, text: label }), control];
  }
  return [h("label", { text: label }), control];
};

// ---------------------------------------------------------------- insurance --
function insuranceFields(optional) {
  const has = h("input", { type: "checkbox", name: "hasInsurance" });
  const insurer = h("input", { name: "insurer" });
  const policy = h("input", { name: "policy" });
  const kind = h("select", { name: "coverKind" }, option("flat", "Flat amount"), option("percent", "Percentage of the bill"));
  const amount = h("input", { name: "amount", type: "number", min: "0", step: "0.01", value: "0" });
  const percent = h("input", { name: "percent", type: "number", min: "0", max: "100", step: "0.1", value: "0" });
  const cap = h("input", { name: "cap", type: "number", min: "0", step: "0.01", value: "0", title: "0 = no limit" });
  const rows = { amount: row("Amount covered (Rs)", amount), percent: row("Percentage covered", percent), cap: row("Maximum payout (Rs, 0 = no limit)", cap) };
  const update = () => {
    const on = !optional || has.checked;
    [insurer, policy, kind, amount, percent, cap].forEach(el => (el.disabled = !on));
    const flat = kind.value === "flat";
    rows.amount.forEach(el => (el.hidden = !flat));
    rows.percent.forEach(el => (el.hidden = flat));
    rows.cap.forEach(el => (el.hidden = flat));
  };
  has.addEventListener("change", update);
  kind.addEventListener("change", update);
  const element = h("fieldset", {}, h("legend", { text: "Insurance" }), h("div", { class: "form" },
    optional ? [h("span"), h("label", {}, has, "Patient has insurance")] : null,
    row("Insurer", insurer), row("Policy number", policy), row("Cover type", kind), rows.amount, rows.percent, rows.cap));
  update();
  return {
    element,
    values() {
      if (optional && !has.checked) return {};
      return { insurer: insurer.value.trim(), policy: policy.value.trim(), coverKind: kind.value,
               amount: amount.value, percent: percent.value, cap: cap.value };
    },
    wanted: () => !optional || has.checked,
    reset() { has.checked = false; [insurer, policy].forEach(el => (el.value = "")); kind.value = "flat"; [amount, percent, cap].forEach(el => (el.value = "0")); update(); },
  };
}

// ----------------------------------------------------------------- queues --
export function queuesPanel({ compact = false } = {}) {
  let selected = null;
  const er = h("div", { class: "table-wrap" });
  const opd = h("div", { class: "table-wrap" });
  const refresh = async () => {
    const q = await api.get("/api/queues");
    const erCols = [
      { key: "position", label: "#" }, { key: "id", label: "Patient ID", mono: true }, { key: "name", label: "Name" },
      ...(compact ? [] : [{ key: "age", label: "Age" }]),
      { label: "ESI", render: r => esiBadge(r.severity, r.esiName) },
      ...(compact ? [] : [{ key: "complaint", label: "Complaint" }, { key: "registered", label: "Registered" }]),
    ];
    const opdCols = [
      { key: "position", label: "#" }, { key: "id", label: "Patient ID", mono: true }, { key: "name", label: "Name" },
      ...(compact ? [] : [{ key: "age", label: "Age" }]), { key: "department", label: "Department" },
      ...(compact ? [] : [{ key: "reason", label: "Reason for visit" }]),
    ];
    const pick = r => { selected = r.id; refresh(); };
    er.replaceChildren(table(erCols, q.emergency, { keyOf: r => r.id, selected, onSelect: pick, empty: "No emergency patient is waiting." }));
    opd.replaceChildren(table(opdCols, q.opd, { keyOf: r => r.id, selected, onSelect: pick, empty: "No outpatient is waiting." }));
  };
  live.on(["queueChanged", "patientsChanged", "reloaded"], refresh);
  refresh();
  const element = h("div", {},
    section(compact ? "Emergency queue" : "Emergency queue, most urgent first",
            compact ? null : "Order from the triage heap: lower ESI level first, then earlier arrival.", er),
    section(compact ? "OPD line" : "OPD line, first come first served",
            compact ? null : "Outpatients are seen in arrival order once no emergency is waiting.", opd));
  return { element, select(id) { selected = id; refresh(); } };
}

// ----------------------------------------------------------- registration --
export function registrationPanel() {
  const queues = queuesPanel();
  const emergencyForm = () => {
    const insurance = insuranceFields(true);
    const form = h("form", { class: "pad stack", novalidate: true },
      h("div", { class: "form" },
        row("Name", h("input", { name: "name", autocomplete: "off" })),
        row("Age", h("input", { name: "age", type: "number", min: "0", max: "130" })),
        row("Phone", h("input", { name: "phone", placeholder: "Optional" })),
        row("Presenting complaint", h("input", { name: "complaint" }))),
      h("fieldset", {}, h("legend", { text: "Severity (Emergency Severity Index)" }),
        h("div", { class: "esi-choice" }, ESI.map(([lvl, name, desc]) => [
          h("span", { class: `esi l${lvl}`, text: String(lvl) }),
          h("label", {}, h("input", { type: "radio", name: "severity", value: String(lvl) }), name),
          h("span", { class: "hint", text: desc })])),
        h("p", { class: "hint", text: "ESI 1-2 are admitted to an ICU bed, ESI 3 to a ward bed; ESI 4-5 are treated in the emergency room and released." })),
      insurance.element,
      h("div", {}, h("button", { class: "btn primary", type: "submit", text: "Register patient" })));
    form.addEventListener("submit", async (e) => {
      e.preventDefault();
      const r = await attempt(() => api.post("/api/register-emergency", {
        name: field(form, "name"), age: field(form, "age"), phone: field(form, "phone"), complaint: field(form, "complaint"),
        severity: form.querySelector('input[name="severity"]:checked')?.value ?? "", ...insurance.values() }));
      if (r) { form.reset(); insurance.reset(); queues.select(r.patientId); form.elements.name.focus(); }
    });
    return form;
  };
  const opdForm = () => {
    const insurance = insuranceFields(true);
    const dept = h("select", { name: "department" });
    api.get("/api/doctors").then(d => dept.replaceChildren(...(d.opdDepartments.length ? d.opdDepartments : ["General Medicine"]).map(x => option(x))));
    const form = h("form", { class: "pad stack", novalidate: true },
      h("div", { class: "form" },
        row("Name", h("input", { name: "name", autocomplete: "off" })),
        row("Age", h("input", { name: "age", type: "number", min: "0", max: "130" })),
        row("Phone", h("input", { name: "phone", placeholder: "Optional" })),
        row("Reason for visit", h("input", { name: "complaint" })),
        row("Department", dept)),
      insurance.element,
      h("div", {}, h("button", { class: "btn primary", type: "submit", text: "Register patient" })));
    form.addEventListener("submit", async (e) => {
      e.preventDefault();
      const r = await attempt(() => api.post("/api/register-opd", {
        name: field(form, "name"), age: field(form, "age"), phone: field(form, "phone"), complaint: field(form, "complaint"),
        department: dept.value, ...insurance.values() }));
      if (r) { const d = dept.value; form.reset(); dept.value = d; insurance.reset(); queues.select(r.patientId); }
    });
    return form;
  };
  const forms = tabs([{ label: "Emergency patient", build: emergencyForm }, { label: "Outpatient (OPD)", build: opdForm }]);
  return h("div", { class: "cols" }, h("div", {}, forms.element), queues.element);
}

// ----------------------------------------------------------- transfer desk --
export function transferPanel() {
  const pending = h("div", { class: "table-wrap" });
  const form = h("form", { class: "pad stack", novalidate: true },
    h("fieldset", {}, h("legend", { text: "New request" }), h("div", { class: "form" },
      h("label", { text: "Priority" }), h("div", {},
        h("label", {}, h("input", { type: "radio", name: "urgent", value: "1" }), "Urgent: front of the line"), h("br"),
        h("label", {}, h("input", { type: "radio", name: "urgent", value: "0", checked: true }), "Routine: back of the line")),
      h("label", { text: "Type" }), h("div", {},
        h("label", {}, h("input", { type: "radio", name: "ambulance", value: "1", checked: true }), "Ambulance"), h("br"),
        h("label", {}, h("input", { type: "radio", name: "ambulance", value: "0" }), "Internal transfer")),
      row("Patient ID", h("input", { name: "patientId", class: "id", autocapitalize: "characters", spellcheck: "false", autocomplete: "off", placeholder: "Blank if not registered yet" })),
      row("From", h("input", { name: "from" })),
      row("To", h("input", { name: "to" })))),
    h("div", {}, h("button", { class: "btn primary", type: "submit", text: "Add request" })),
    h("p", { class: "hint", text: "Dispatching an ambulance run for a registered patient adds the ambulance charge to their open bill." }));
  form.addEventListener("submit", async (e) => {
    e.preventDefault();
    const r = await attempt(() => api.post("/api/transfer", {
      urgent: form.querySelector('input[name="urgent"]:checked').value, ambulance: form.querySelector('input[name="ambulance"]:checked').value,
      patientId: field(form, "patientId"), from: field(form, "from"), to: field(form, "to") }));
    if (r) { form.reset(); }
  });
  const refresh = async () => {
    const t = await api.get("/api/transfers");
    pending.replaceChildren(table([
      { key: "position", label: "#" }, { key: "code", label: "Code", mono: true }, { key: "priority", label: "Priority" },
      { key: "type", label: "Type" }, { label: "Patient ID", render: r => r.patientId ? h("span", { class: "mono", text: r.patientId }) : "Not registered" },
      { key: "from", label: "From" }, { key: "to", label: "To" }], t.pending, { empty: "No pending requests." }));
  };
  live.on(["transfersChanged", "reloaded"], refresh);
  refresh();
  return h("div", { class: "cols" }, h("div", {}, form),
    section("Pending requests, dispatched from the top", "Urgent requests join at the front, routine ones at the back.", pending,
      h("div", { class: "row" },
        h("button", { class: "btn", type: "button", onclick: () => attempt(() => api.post("/api/transfer/dispatch")), text: "Dispatch next request" }),
        h("button", { class: "btn", type: "button", onclick: () => attempt(() => api.post("/api/transfer/cancel")), text: "Cancel newest routine request" }))));
}

// ---------------------------------------------------------------- treatment --
export function treatPanel() {
  const next = h("div", { class: "record" });
  const notes = h("form", { novalidate: true });
  const result = h("div", { class: "result", text: "No patient treated on this page yet." });
  const map = routeMap({ vertical: true });
  const button = h("button", { class: "btn primary", type: "submit", text: "Treat next patient" });
  const opdBox = h("fieldset", { hidden: true }, h("legend", { text: "Outpatient consultation" }), h("div", { class: "form" },
    row("Diagnosis", h("input", { name: "diagnosis", placeholder: "Routine review" })),
    row("Prescriptions", h("input", { name: "prescriptions", placeholder: "None" })),
    row("Medicine to dispense", h("input", { name: "medicine", placeholder: "Blank for none" })),
    row("Quantity", h("input", { name: "quantity", type: "number", min: "1", value: "1" })),
    row("Unit price (Rs)", h("input", { name: "unitPrice", type: "number", min: "0", step: "0.01", value: "0" }))));
  notes.append(h("div", { class: "stack" }, opdBox, h("div", {}, button)));
  const refreshNext = async () => {
    const s = await api.get("/api/state");
    const n = s.next;
    button.disabled = !n;
    opdBox.hidden = !(n && n.kind === "opd");
    next.replaceChildren(...(n ? [
      h("dt", { text: "Patient ID" }), h("dd", { class: "mono", text: n.id }),
      h("dt", { text: "Name" }), h("dd", { text: `${n.name}, ${n.age}` }),
      h("dt", { text: "Triage" }), h("dd", {}, n.kind === "emergency" ? esiBadge(n.severity, `Emergency, ESI ${n.severity} ${n.esiName}`) : `Outpatient, ${n.department}`),
      h("dt", { text: "Complaint" }), h("dd", { text: n.complaint }),
      h("dt", { text: "Care" }), h("dd", { text: n.needs }),
    ] : [h("dt", { text: "Nobody is waiting." }), h("dd")]));
  };
  notes.addEventListener("submit", async (e) => {
    e.preventDefault();
    const r = await attempt(() => api.post("/api/treat-next", {
      diagnosis: field(notes, "diagnosis"), prescriptions: field(notes, "prescriptions"),
      medicine: field(notes, "medicine"), quantity: field(notes, "quantity"), unitPrice: field(notes, "unitPrice") }));
    if (!r) return;
    result.textContent = r.lines.join("\n");
    notes.reset();
    if (r.outcome === "admitted") map.setRoute(await api.get(`/api/route?id=${encodeURIComponent(r.patientId)}`).catch(() => null));
    else map.setRoute(null, r.outcome === "outpatient" ? "Outpatients are not given a bed, so there is no route."
                        : r.outcome === "released" ? "Treated in the emergency room, so there is no route."
                        : "No bed was assigned, so there is no route.");
  });
  live.on(["queueChanged", "patientsChanged", "bedsChanged", "reloaded"], refreshNext);
  refreshNext();
  const queues = queuesPanel({ compact: true });
  return h("div", { class: "cols3" },
    h("div", { class: "pad stack" }, h("fieldset", {}, h("legend", { text: "Next patient" }), next), notes,
      h("fieldset", {}, h("legend", { text: "Last treatment" }), result)),
    h("div", { class: "pad" }, map.element),
    queues.element);
}

// --------------------------------------------------------------- record view --
// One patient's record and visit history (Doctor, Super Admin, Patient).
export function recordView() {
  let id = "";
  const title = h("div", { class: "record-title", text: "No patient selected" });
  const fields = h("dl", { class: "record" });
  const visits = h("div", { class: "table-wrap" });
  const refresh = async () => {
    if (!id) { title.textContent = "No patient selected"; fields.replaceChildren(); visits.replaceChildren(); return; }
    let p;
    try { p = await api.get(`/api/patient?id=${encodeURIComponent(id)}`); }
    catch (e) { title.textContent = `${id} is not registered`; fields.replaceChildren(); visits.replaceChildren(); return; }
    title.replaceChildren(p.kind === "emergency" ? h("span", { class: `esi l${p.severity}`, text: String(p.severity) }) : "", `${p.name}, ${p.age}`);
    const dd = (label, value, mono) => [h("dt", { text: label }), h("dd", { class: mono ? "mono" : null, text: value })];
    fields.replaceChildren(
      ...dd("Patient ID", p.id, true),
      ...dd("Type", p.kind === "emergency" ? `Emergency, ESI ${p.severity} ${p.esiName}` : `Outpatient, ${p.department}`),
      ...dd("Status", p.status), ...dd("Complaint", p.complaint), ...dd("Registered", p.registered),
      ...dd("Bed", p.bed || "None"),
      ...dd("Doctor", p.doctor ? `${p.doctor.name} (${p.doctor.specialty})` : "Not assigned yet"),
      ...dd("Discharged", p.discharged || "Not discharged"),
      ...dd("Bill", p.bill ? `${p.bill.billId}, ${p.bill.totalText} ${p.bill.settled ? "(settled)" : "due"}` : "No bill"),
      ...dd("Insurance", p.insurance || "None"));
    visits.replaceChildren(table([{ key: "date", label: "Date" }, { key: "doctor", label: "Doctor" },
      { key: "diagnosis", label: "Diagnosis" }, { key: "prescriptions", label: "Prescriptions" }], p.visits, { empty: "No visits recorded." }));
  };
  live.on(["patientsChanged", "billsChanged", "bedsChanged", "staffChanged", "reloaded"], refresh);
  return {
    element: h("div", { class: "stack" }, title, fields,
      h("div", {}, h("h2", { text: "Visit history, newest first" }), h("div", { class: "hint", text: "Read from the patient's doubly linked list, walked from the tail." })), visits),
    show(newId) { id = newId; return refresh(); },
    get id() { return id; },
  };
}

// ------------------------------------------------------------ patient lookup --
export function lookupPanel() {
  const record = recordView();
  const stats = h("div", { class: "hint" });
  const search = h("form", { class: "row", novalidate: true },
    h("label", { text: "Patient ID" }), h("input", { name: "id", class: "id", autocapitalize: "characters", spellcheck: "false", autocomplete: "off", "aria-label": "Patient ID", placeholder: "VR-2026-000001", style: "max-width: 16em" }),
    h("button", { class: "btn primary", type: "submit", text: "Look up" }));
  const doctors = h("datalist", { id: "doctor-names" });
  api.get("/api/doctors").then(d => doctors.replaceChildren(...d.availability.map(a => option(a.name))));
  const note = h("form", { novalidate: true }, h("fieldset", {}, h("legend", { text: "Add a visit note" }), h("div", { class: "form" },
    row("Visit date", h("input", { name: "date", type: "date", value: today() })),
    row("Doctor", h("input", { name: "doctor", list: "doctor-names" })),
    row("Diagnosis", h("input", { name: "diagnosis" })),
    row("Prescriptions", h("input", { name: "prescriptions", placeholder: "None" })),
    h("span"), h("div", {}, h("button", { class: "btn", type: "submit", text: "Add visit note" })))), doctors,
    h("p", { class: "hint", text: "Notes go into the visit history in date order: at the head or tail in one step, otherwise walked into place." }));
  search.addEventListener("submit", async (e) => {
    e.preventDefault();
    const id = field(search, "id");
    if (!id) { message("Enter a patient ID, for example VR-2026-000001.", true); return; }
    const p = await attempt(() => api.get(`/api/patient?id=${encodeURIComponent(id)}`));
    if (p) { message(""); record.show(p.id);
      stats.textContent = `Patient ID index (hash table): ${p.hashIndex.entries} entries in ${p.hashIndex.buckets} buckets, load ${Number(p.hashIndex.load).toFixed(2)}, ${p.hashIndex.collisions} collisions, ${p.hashIndex.resizes} resizes.`; }
  });
  note.addEventListener("submit", async (e) => {
    e.preventDefault();
    if (!record.id) { message("Look up a patient first.", true); return; }
    const r = await attempt(() => api.post("/api/visit-note", { id: record.id, date: field(note, "date"), doctor: field(note, "doctor"),
      diagnosis: field(note, "diagnosis"), prescriptions: field(note, "prescriptions") }));
    if (r) { note.elements.diagnosis.value = ""; note.elements.prescriptions.value = ""; }
  });
  return h("div", { class: "cols" }, h("div", { class: "pad stack" }, search, stats, record.element), h("div", { class: "pad" }, note));
}

// ----------------------------------------------------------------- theatres --
export function theatresPanel() {
  let selected = null;
  const theatreSelect = h("select", { name: "theatre" });
  const rings = h("div", { class: "mono" });
  const slots = h("div", { class: "table-wrap tall" });
  const form = h("form", { class: "pad stack", novalidate: true },
    h("fieldset", {}, h("legend", { text: "Book the next free slot" }), h("div", { class: "form" },
      row("Theatre", theatreSelect),
      row("Patient ID", h("input", { name: "patientId", class: "id", autocapitalize: "characters", spellcheck: "false", autocomplete: "off", placeholder: "VR-2026-000004" })),
      row("Procedure", h("input", { name: "procedure" })),
      row("Surgeon's fee (Rs)", h("input", { name: "fee", type: "number", min: "0", step: "0.01", value: "0" })))),
    h("div", {}, h("button", { class: "btn primary", type: "submit", text: "Book next free slot" })),
    h("p", { class: "hint", text: "Booking charges the surgery to the patient's bill: surgeon's fee, Rs 8000.00 theatre fee and Rs 3500.00 consumables." }));
  form.addEventListener("submit", async (e) => {
    e.preventDefault();
    const r = await attempt(() => api.post("/api/theatre/book", { theatre: theatreSelect.value, patientId: field(form, "patientId"),
      procedure: field(form, "procedure"), fee: field(form, "fee") }));
    if (r) { const t = theatreSelect.value; form.reset(); theatreSelect.value = t; }
  });
  const refresh = async () => {
    const t = await api.get("/api/theatres");
    const keep = theatreSelect.value;
    theatreSelect.replaceChildren(...t.theatres.map(x => option(x.name)));
    if (keep) theatreSelect.value = keep;
    rings.replaceChildren(...t.theatres.map(x => h("div", { text: `${x.name}: ${x.freeCount} of ${x.total} slots free. Next free in cycle order: ${x.ring.length ? x.ring.join(" ") : "none (fully booked)"}` })));
    const rows = t.theatres.flatMap(x => x.slots.map(s => ({ ...s, theatre: x.name, key: `${x.name}|${s.slot}` })));
    slots.replaceChildren(table([{ key: "theatre", label: "Theatre" }, { key: "slot", label: "Slot" }, { key: "time", label: "Time", mono: true },
      { label: "Status", render: r => r.booked ? "Booked" : "Free" }, { key: "patientId", label: "Patient ID", mono: true },
      { key: "patientName", label: "Patient" }, { key: "procedure", label: "Procedure" }], rows,
      { keyOf: r => r.key, selected, onSelect: r => { selected = r.key; refresh(); } }));
  };
  live.on(["theatresChanged", "patientsChanged", "reloaded"], refresh);
  refresh();
  const release = () => {
    if (!selected) { message("Select a booked slot in the schedule first.", true); return; }
    const [theatre, slot] = selected.split("|");
    attempt(() => api.post("/api/theatre/release", { theatre, slot }));
  };
  return h("div", { class: "cols" }, h("div", {}, form),
    section("Today's theatre schedule", "Free slots wait in a circular queue per theatre. A released slot joins the back of the ring and can be booked again.",
      rings, slots, h("div", {}, h("button", { class: "btn", type: "button", onclick: release, text: "Release selected slot" }))));
}

// ----------------------------------------------------------- doctors on duty --
export function doctorsPanel() {
  const spec = h("select", { name: "specialty", "aria-label": "Specialty" });
  const result = h("div", { text: "Choose a specialty." });
  const list = h("div", { class: "table-wrap" });
  const find = async (e) => {
    e?.preventDefault();
    const r = await attempt(() => api.get(`/api/next-doctor?specialty=${encodeURIComponent(spec.value)}`));
    if (r) { result.textContent = r.text; message(r.text); }
  };
  const refresh = async () => {
    const d = await api.get("/api/doctors");
    const keep = spec.value;
    spec.replaceChildren(...d.specialties.map(s => option(s)));
    if (keep) spec.value = keep;
    list.replaceChildren(table([{ key: "specialty", label: "Specialty" }, { key: "name", label: "Doctor" }, { key: "id", label: "Staff ID", mono: true },
      { key: "status", label: "Next free" }, { key: "seen", label: "Patients seen" }], d.availability));
  };
  live.on(["staffChanged", "reloaded"], refresh);
  refresh();
  const form = h("form", { class: "row", onsubmit: find, novalidate: true }, h("label", { text: "Specialty" }), spec,
    h("button", { class: "btn primary", type: "submit", text: "Find next available doctor" }));
  return section("Doctors on duty", null, form, result,
    h("div", {}, h("h3", { text: "Availability array" }), h("div", { class: "hint", text: "Kept sorted by specialty, then next-free time. The next available doctor is found with a lower-bound binary search." })), list);
}

// ------------------------------------------------------------------- bill --
export function billView() {
  let id = "";
  const header = h("h2", { text: "No bill to show" });
  const lines = h("div", { class: "table-wrap" });
  const totals = h("dl", { class: "totals", hidden: true });
  const refresh = async () => {
    if (!id) { header.textContent = "No bill to show"; lines.replaceChildren(); totals.hidden = true; return; }
    let b;
    try { b = await api.get(`/api/bill?id=${encodeURIComponent(id)}`); }
    catch (e) { header.textContent = "No bill to show"; lines.replaceChildren(); totals.hidden = true; return; }
    header.textContent = `Bill ${b.billId}, opened ${b.opened}, ${b.settled ? `settled on ${b.settledOn}` : "open"}`;
    lines.replaceChildren(table([{ key: "date", label: "Date" }, { key: "category", label: "Type" }, { key: "description", label: "Description" },
      { key: "detail", label: "Calculation" }, { key: "amountText", label: "Amount", mono: true, right: true }], b.lines, { empty: "No charges yet." }));
    totals.hidden = false;
    totals.replaceChildren(h("dt", { text: "Subtotal" }), h("dd", { class: "mono", text: b.subtotalText }),
      h("dt", { text: "Insurance" }), h("dd", { text: b.insurance ? `${b.insurance}. ${b.settled ? "Deducted" : "Deducted once, at discharge: about"} ${b.deductionText}` : "None recorded" }),
      h("dt", { class: "strong", text: "Total" }), h("dd", { class: "strong", text: `${b.settled ? "Paid" : "Due"} ${b.totalText}${b.settled ? "" : " (estimate until discharge)"}` }));
  };
  live.on(["billsChanged", "reloaded"], refresh);
  return { element: h("div", { class: "stack" }, header, lines, totals), show(newId) { id = newId; return refresh(); }, get id() { return id; } };
}

// ------------------------------------------------------- billing, discharge --
export function billingPanel() {
  const bill = billView();
  const all = h("div", { class: "table-wrap" });
  const sort = h("select", { "aria-label": "Sort bills by" }, option("date", "Date opened"), option("amount", "Amount, largest first"));
  const authoriser = h("select", { name: "authoriser" });
  const type = h("select", { name: "type" }, option("bed", "Bed"), option("medicine", "Medicine"), option("test", "Test"), option("procedure", "Procedure"));
  const kinds = {
    bed: [row("Rate per day (Rs)", h("input", { name: "rate", type: "number", min: "0", step: "0.01", value: "0" })), row("Days", h("input", { name: "days", type: "number", min: "1", value: "1" }))],
    medicine: [row("Unit price (Rs)", h("input", { name: "unitPrice", type: "number", min: "0", step: "0.01", value: "0" })), row("Quantity", h("input", { name: "quantity", type: "number", min: "1", value: "1" }))],
    test: [row("Price (Rs)", h("input", { name: "price", type: "number", min: "0", step: "0.01", value: "0" })), [h("span"), h("label", {}, h("input", { type: "checkbox", name: "urgent" }), "Urgent (+25%)")]],
    procedure: [row("Professional fee (Rs)", h("input", { name: "fee", type: "number", min: "0", step: "0.01", value: "0" })),
                row("Facility fee (Rs)", h("input", { name: "facility", type: "number", min: "0", step: "0.01", value: "0" })),
                row("Consumables (Rs)", h("input", { name: "consumables", type: "number", min: "0", step: "0.01", value: "0" }))],
  };
  const showKind = () => Object.entries(kinds).forEach(([k, rows]) => rows.flat().forEach(el => (el.hidden = k !== type.value)));
  type.addEventListener("change", showKind);
  const idForm = h("form", { class: "row", novalidate: true }, h("label", { text: "Patient ID" }),
    h("input", { name: "id", class: "id", autocapitalize: "characters", spellcheck: "false", autocomplete: "off", "aria-label": "Patient ID", placeholder: "VR-2026-000004", style: "max-width: 16em" }), h("button", { class: "btn primary", type: "submit", text: "Open bill" }));
  const charge = h("form", { novalidate: true }, h("fieldset", {}, h("legend", { text: "Add a manual charge" }), h("div", { class: "form" },
    row("Authorised by", authoriser), row("Charge type", type), row("Description", h("input", { name: "description" })),
    Object.values(kinds).flat(), h("span"), h("div", {}, h("button", { class: "btn", type: "submit", text: "Add charge" })))));
  const insurance = insuranceFields(false);
  const insForm = h("form", { novalidate: true }, insurance.element, h("div", { style: "margin-top: 8px" }, h("button", { class: "btn", type: "submit", text: "Record insurance" })));
  const discharge = h("form", { novalidate: true }, h("fieldset", {}, h("legend", { text: "Discharge and settle" }), h("div", { class: "form" },
    row("Discharge date", h("input", { name: "date", type: "date", value: today() })),
    h("span"), h("div", {}, h("button", { class: "btn", type: "button", text: "Discharge and settle bill", onclick: () => doDischarge() })),
    h("p", { class: "hint full", text: "Bed days after the first are charged up to the discharge date, the bed is released and insurance is deducted once." }))));
  const editors = [charge, insForm, discharge];
  const setEditable = (open) => editors.forEach(f => f.querySelectorAll("input, select, button").forEach(el => (el.disabled = !open)));
  const open = async (id) => {
    if (!id) { message("Enter a patient ID.", true); return; }
    const b = await attempt(() => api.get(`/api/bill?id=${encodeURIComponent(id)}`));
    if (!b) return;
    idForm.elements.id.value = b.patientId;
    await bill.show(b.patientId);
    setEditable(!b.settled);
    message(b.settled ? `Bill ${b.billId} is settled and can no longer be changed.` : "");
  };
  idForm.addEventListener("submit", (e) => { e.preventDefault(); open(field(idForm, "id")); });
  charge.addEventListener("submit", async (e) => {
    e.preventDefault();
    if (!bill.id) { message("Open a bill first.", true); return; }
    const values = { id: bill.id, authoriser: authoriser.value, type: type.value, description: field(charge, "description") };
    kinds[type.value].flat().forEach(el => { const inp = el.querySelector ? (el.matches("input") ? el : el.querySelector("input")) : null;
      if (inp) values[inp.name] = inp.type === "checkbox" ? (inp.checked ? "1" : "0") : inp.value; });
    const r = await attempt(() => api.post("/api/charge", values));
    if (r) charge.elements.description.value = "";
  });
  insForm.addEventListener("submit", async (e) => {
    e.preventDefault();
    if (!bill.id) { message("Open a bill first.", true); return; }
    const r = await attempt(() => api.post("/api/insurance", { id: bill.id, ...insurance.values() }));
    if (r) insurance.reset();
  });
  const doDischarge = async () => {
    if (!bill.id) { message("Open a bill first.", true); return; }
    const p = await api.get(`/api/patient?id=${encodeURIComponent(bill.id)}`).catch(() => null);
    if (!p) return;
    if (p.status === "Waiting") { message(`${p.id} is still waiting to be seen.`); return; }
    if (p.status === "Discharged") { message(`${p.id} was already discharged.`); return; }
    const date = field(discharge, "date");
    if (!window.confirm(`Discharge ${p.id} ${p.name} on ${date} and settle bill ${p.bill ? p.bill.billId : ""}?\n\n` +
        (p.bed ? `${p.bed} is released; bed days after the first are charged up to the discharge date.\n` : "") +
        (p.insurance ? `Insurance deducted once: ${p.insurance}.\n` : "No insurance is recorded; the full amount is due.\n") +
        "\nThis cannot be undone.")) return;
    const r = await attempt(() => api.post("/api/discharge", { id: p.id, date }));
    if (r) setEditable(false);
  };
  const refreshAll = async () => {
    const b = await api.get(`/api/bills?sort=${sort.value}`);
    all.replaceChildren(table([{ key: "billId", label: "Bill", mono: true }, { key: "patientId", label: "Patient ID", mono: true },
      { key: "opened", label: "Opened" }, { key: "totalText", label: "Total", mono: true, right: true },
      { label: "State", render: r => (r.settled ? "Settled" : "Open") }], b.bills,
      { keyOf: r => r.patientId, selected: bill.id, onSelect: r => open(r.patientId) }));
  };
  sort.addEventListener("change", refreshAll);
  api.get("/api/staff").then(s => authoriser.replaceChildren(...s.staff.filter(x => x.canAuthoriseCharges).map(x => option(x.id, `${x.id}  ${x.name}`))));
  live.on(["billsChanged", "reloaded"], refreshAll);
  refreshAll();
  showKind();
  setEditable(false);
  return h("div", { class: "cols" },
    h("div", { class: "pad stack" }, idForm, charge, insForm, discharge),
    h("div", {}, h("div", { class: "pad" }, bill.element),
      section("All bills", "Merge-sorted (stable). Click a bill to open it.", h("div", { class: "row" }, h("label", { text: "Sort by" }), sort), all)));
}

// --------------------------------------------------------------- admissions --
export function admissionsPanel() {
  const d = new Date(); d.setDate(d.getDate() - 30);
  const from = h("input", { type: "date", name: "from", "aria-label": "From date", value: `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, "0")}-${String(d.getDate()).padStart(2, "0")}` });
  const to = h("input", { type: "date", name: "to", "aria-label": "To date", value: today() });
  const summary = h("div");
  const avl = h("div", { class: "hint" });
  const list = h("div", { class: "table-wrap tall" });
  // `report` is false for live refreshes: they must not clear or overwrite
  // the status bar message of whatever the user just did.
  const load = async (report) => {
    let a;
    try { if (report) message(""); a = await api.get(`/api/admissions?from=${from.value}&to=${to.value}`); }
    catch (err) { if (report) message(err.message, true); return; }
    from.value = a.from; to.value = a.to;
    summary.textContent = a.summary; avl.textContent = a.avl;
    list.replaceChildren(table([{ key: "registered", label: "Registered" }, { key: "id", label: "Patient ID", mono: true },
      { key: "name", label: "Name" }, { key: "kind", label: "Type" }, { key: "status", label: "Status" }], a.patients));
  };
  const show = (e) => { e?.preventDefault(); load(true); };
  live.on(["patientsChanged", "reloaded"], () => load(false));
  load(false);
  return section("Registrations in the range, in date order", "Found by an in-order walk of the admission-date AVL tree that skips subtrees outside the range.",
    h("form", { class: "row", onsubmit: show, novalidate: true }, h("label", { text: "From" }), from, h("label", { text: "To" }), to,
      h("button", { class: "btn primary", type: "submit", text: "Show admissions" })), summary, avl, list);
}

// ------------------------------------------------------------------- staff --
export function staffPanel() {
  const list = h("div", { class: "table-wrap" });
  api.get("/api/staff").then(s => list.replaceChildren(table([{ key: "id", label: "Staff ID", mono: true }, { key: "name", label: "Name" },
    { key: "role", label: "Role" }, { key: "department", label: "Department" }, { key: "shift", label: "Shift" }, { key: "detail", label: "Details" }], s.staff)));
  return h("div", {}, section("Staff roster", "Doctors, nurses and administrators share the Staff base class; only administrators may authorise manual charges.", list), doctorsPanel());
}

// ----------------------------------------------------------------- indexes --
export function indexesPanel() {
  const hash = h("dl", { class: "record mono" });
  const tree = h("dl", { class: "record mono" });
  const refresh = async () => {
    const x = await api.get("/api/indexes");
    const dd = (label, v) => [h("dt", { text: label }), h("dd", { text: String(v) })];
    hash.replaceChildren(...dd("Entries", x.hash.entries), ...dd("Buckets", x.hash.buckets), ...dd("Buckets in use", x.hash.usedBuckets),
      ...dd("Load factor", Number(x.hash.load).toFixed(2)), ...dd("Longest chain", x.hash.longestChain), ...dd("Collisions", x.hash.collisions), ...dd("Resizes", x.hash.resizes));
    tree.replaceChildren(...dd("Distinct dates (nodes)", x.avl.nodes), ...dd("Patients stored", x.avl.values), ...dd("Height", x.avl.height),
      ...dd("Rotations LL / RR", `${x.avl.ll} / ${x.avl.rr}`), ...dd("Rotations LR / RL", `${x.avl.lr} / ${x.avl.rl}`), ...dd("Balanced (checked now)", x.avl.balanced ? "Yes" : "No"));
  };
  live.on(["patientsChanged", "reloaded"], refresh);
  refresh();
  return h("div", { class: "cols" },
    h("div", { class: "pad" }, h("fieldset", {}, h("legend", { text: "Patient ID index (hash table, separate chaining)" }), hash,
      h("p", { class: "hint", text: "FNV-1a hash. The bucket array doubles before the load factor would pass its limit. A collision is an insert into a bucket that already holds an entry." }))),
    h("div", { class: "pad" }, h("fieldset", {}, h("legend", { text: "Admission-date index (AVL tree)" }), tree,
      h("p", { class: "hint", text: "Several patients can share a date, so each node holds a list. Balance is restored after every insert and removal." }))));
}

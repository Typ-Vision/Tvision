---
name: Precision Machine Vision Studio
colors:
  surface: '#f8f9ff'
  surface-dim: '#cbdbf5'
  surface-bright: '#f8f9ff'
  surface-container-lowest: '#ffffff'
  surface-container-low: '#eff4ff'
  surface-container: '#e5eeff'
  surface-container-high: '#dce9ff'
  surface-container-highest: '#d3e4fe'
  on-surface: '#0b1c30'
  on-surface-variant: '#434655'
  inverse-surface: '#213145'
  inverse-on-surface: '#eaf1ff'
  outline: '#737686'
  outline-variant: '#c3c6d7'
  surface-tint: '#0053db'
  primary: '#004ac6'
  on-primary: '#ffffff'
  primary-container: '#2563eb'
  on-primary-container: '#eeefff'
  inverse-primary: '#b4c5ff'
  secondary: '#006a61'
  on-secondary: '#ffffff'
  secondary-container: '#86f2e4'
  on-secondary-container: '#006f66'
  tertiary: '#4d556b'
  on-tertiary: '#ffffff'
  tertiary-container: '#656d84'
  on-tertiary-container: '#eef0ff'
  error: '#ba1a1a'
  on-error: '#ffffff'
  error-container: '#ffdad6'
  on-error-container: '#93000a'
  primary-fixed: '#dbe1ff'
  primary-fixed-dim: '#b4c5ff'
  on-primary-fixed: '#00174b'
  on-primary-fixed-variant: '#003ea8'
  secondary-fixed: '#89f5e7'
  secondary-fixed-dim: '#6bd8cb'
  on-secondary-fixed: '#00201d'
  on-secondary-fixed-variant: '#005049'
  tertiary-fixed: '#dae2fd'
  tertiary-fixed-dim: '#bec6e0'
  on-tertiary-fixed: '#131b2e'
  on-tertiary-fixed-variant: '#3f465c'
  background: '#f8f9ff'
  on-background: '#0b1c30'
  surface-variant: '#d3e4fe'
typography:
  headline-lg:
    fontFamily: Inter
    fontSize: 20px
    fontWeight: '600'
    lineHeight: 28px
  headline-md:
    fontFamily: Inter
    fontSize: 16px
    fontWeight: '600'
    lineHeight: 24px
  headline-sm:
    fontFamily: Inter
    fontSize: 14px
    fontWeight: '600'
    lineHeight: 20px
  body-lg:
    fontFamily: Inter
    fontSize: 13px
    fontWeight: '500'
    lineHeight: 18px
  body-md:
    fontFamily: Inter
    fontSize: 12px
    fontWeight: '400'
    lineHeight: 16px
  body-sm:
    fontFamily: Inter
    fontSize: 11px
    fontWeight: '400'
    lineHeight: 14px
  label-lg:
    fontFamily: JetBrains Mono
    fontSize: 12px
    fontWeight: '500'
    lineHeight: 16px
  label-md:
    fontFamily: JetBrains Mono
    fontSize: 11px
    fontWeight: '500'
    lineHeight: 14px
  label-sm:
    fontFamily: JetBrains Mono
    fontSize: 10px
    fontWeight: '400'
    lineHeight: 12px
rounded:
  sm: 0.125rem
  DEFAULT: 0.25rem
  md: 0.375rem
  lg: 0.5rem
  xl: 0.75rem
  full: 9999px
spacing:
  gutter: 0.5rem
  margin: 0.75rem
  space-xs: 0.25rem
  space-sm: 0.375rem
  space-md: 0.5rem
  space-lg: 0.75rem
  space-xl: 1rem
---

## Brand & Style

The design system powers next-generation industrial machine vision and automated optical inspection (AOI) software. Tailored for automation engineers, quality control specialists, and computer vision researchers, the interface delivers maximum analytical density while maintaining absolute clarity and optical comfort under long shifts.

The design movement combines **Modern High-Density Technical Software** with **Utilitarian Industrial Precision**—channeling the modular docking ergonomics of Figma and Linear, merged with the fail-safe reliability of industrial factory automation platforms like Cognex VisionPro and Siemens TIA Portal. 

Key attributes:
- **Calibrated & Deterministic:** Every pixel, boundary, and visual token reflects accuracy. No whimsical ornamentation or gratuitous motion.
- **Ergonomic Density:** Maximized screen real estate for live camera viewports, node execution trees, and parametric inspection panels without visual claustrophobia.
- **Industrial Readiness:** Immediate, unambiguous recognition of pass/fail/warning execution states, sensor connectivity, and live frame telemetry.

## Colors

The palette is tuned specifically for high-acuity daylight and factory floor office lighting conditions, moving the application into a luminous, modern engineering workspace:

- **Surface Layers:** Base canvas sits on `#F8FAFC` (Slate 50), elevated docking cards, property panes, and toolbars on crisp `#FFFFFF`, with tool wells and inactive viewports framed on `#F1F5F9`.
- **Structural Outlines:** Fine 1px borders strictly use `#E2E8F0` (Slate 200) for regular partitions and `#CBD5E1` (Slate 300) for active divider splits and canvas borders.
- **Primary Brand (`#2563EB` Electric Blue):** Drives primary actions (Run Inspection, Recipe Deploy), active selection indicators, and focused input states.
- **Secondary (`#0D9488` Precision Teal):** Governs vision tooling primitives (ROI bounding boxes, edge-detection markers, feature extractors, coordinate crosshairs).
- **Industrial Status Signals:**
  - **Pass / Valid / Ready:** `#10B981` (Emerald 500) paired with `#ECFDF5` background tints.
  - **Fail / Defect / Error:** `#EF4444` (Red 500) paired with `#FEF2F2` background tints.
  - **Warning / Degraded / Margin:** `#F59E0B` (Amber 500) paired with `#FFFBEB` background tints.
  - **Offline / Stale:** `#94A3B8` (Slate 400).
- **Text & Data Contrast:** `#0F172A` (Slate 900) delivers ultra-crisp alphanumeric contrast for labels and data values, while `#64748B` handles metadata, units, and inactive states.

## Typography

Typography prioritizes high-density technical readability and numeric tabular alignment:

- **Inter:** Handles all structural hierarchy, window titles, panel headers, button labels, and system status strings. Scaled down to tight 11px–13px ranges standard in desktop engineering IDEs.
- **JetBrains Mono:** Dedicated to numerical precision, coordinates (`X`, `Y`, `Theta`), pixel histograms, frame IDs, exposure times, execution latencies (ms), and DAG pipeline parameters. All numbers must render with tabular figures (`tnum`) to eliminate visual jitter during real-time 60fps streaming inspection.
- **Letter Spacing:** Tightly calibrated negative tracking (`-0.01em`) on Inter headlines to ensure compact headers; zero to slight positive letter spacing on JetBrains Mono for rapid parsing of hex values and floating-point coordinates.

## Layout & Spacing

The layout is built around a multi-pane **Docking Workbench System** optimized for full-HD, 2K, and 4K industrial desktop monitors:

- **Top Navigation & Execution Ribbon:** Fixed 40px height housing workflow control (Recipes, Run Once, Continuous Trigger, Stop), zoom factors, and hardware communication heartbeats (PLC, GigE Vision cameras).
- **Left Panel (Toolbox & Hierarchy):** Width adjustable (default 240px) holding tree views for vision tools (Acquisition, Preprocess, Blob, Code Read, Template Match, Measurement).
- **Center Canvas (Inspection Viewport):** Flexibly expands to consume all available display space. Accommodates calibrated zoom grids, pixel coordinate overlays, and teal/emerald inspection masks.
- **Right Panel (Parametric Inspector):** Width adjustable (default 280px–320px) holding dense key-value inputs, numeric steppers, and conditional drop-downs.
- **Bottom Panel (Diagnostics, I/O & Timeline):** Collapsible drawer (height 160px–240px) displaying DAG flow status, timing logs, and PLC register variables.
- **Component Padding Rhythm:** Uses an ultra-compact 4px/6px/8px rhythm (`space-xs` to `space-md`) within inputs, lists, and status chips to preserve vertical real estate.

## Elevation & Depth

Visual hierarchy relies on **Tonal Layers and Crisp 1px Surface Outlines** rather than soft ambient drop shadows, reflecting professional CAD and vision studio tooling:

- **Level 0 (Application Floor):** `#F8FAFC`.
- **Level 1 (Docked Workspaces & Sidebars):** `#FFFFFF` framed by a continuous `1px solid #E2E8F0` border.
- **Level 2 (Floating Overlays, HUDs & Context Menus):** `#FFFFFF` with a crisp `1px solid #CBD5E1` and an engineering shadow: `0 4px 12px -2px rgba(15, 23, 42, 0.08), 0 2px 4px -1px rgba(15, 23, 42, 0.04)`.
- **Level 3 (Modals & Calibration Overlays):** `#FFFFFF` with `0 20px 25px -5px rgba(15, 23, 42, 0.12)`.
- **Active Focus & Splitters:** Active split-resizers and active panel borders highlight with a sharp `1px` or `2px` stroke in `#2563EB`. Inactive splitters remain an unobtrusive `1px` in `#E2E8F0`.

## Shapes

The design uses **Level 1 (Soft / Precision Chamfer)** corner radiuses. 

- Standard inputs, buttons, tree nodes, and property panels feature a compact `4px` (`0.25rem`) radius.
- Larger docking cards and floating HUD overlays use `6px`–`8px` (`rounded-lg`).
- Status indicator pills and pass/fail indicators remain strictly squared or use minimal `3px` corner softening, eliminating decorative roundness in favor of industrial instrumentation aesthetics.
- Interactive handles on vision viewports (ROI resize anchors, bounding vertices) are sharp `6x6px` squares with `#0D9488` stroke and `#FFFFFF` fill.

## Components

### Buttons & Action Triggers
- **Primary Execution (Run Flow / Trigger):** Height 28px, background `#2563EB`, text `#FFFFFF`, font Inter 12px/600, border radius 4px. Hover: `#1D4ED8`. Active: `#1E40AF`.
- **Secondary / Panel Tools (Fit, Open, Add Tool):** Height 26px, background `#FFFFFF`, border `1px solid #E2E8F0`, text `#0F172A`. Hover: background `#F1F5F9`, border `#CBD5E1`.
- **Stop / Abort:** Height 28px, background `#FEE2E2`, border `1px solid #FCA5A5`, text `#DC2626`. Hover: background `#EF4444`, text `#FFFFFF`.

### Parameter Inputs & Numeric Steppers
- **Compact Property Fields:** Height 24px, padding 2px 6px, font JetBrains Mono 11px. Background `#FFFFFF`, border `1px solid #CBD5E1`. Hover: border `#94A3B8`. Focus: border `#2563EB` with an inner hairline highlight.
- **Increment/Decrement Steppers:** Integrated right-aligned paired chevron arrows within the 24px container, providing single-pixel adjustments for threshold and area settings.

### Tree Lists & Pipeline DAG Nodes
- **Tool Tree Items:** Height 24px, indent 12px per hierarchy depth, hover background `#F8FAFC`. 
- **Selected Node:** Background `#EFF6FF` (Blue 50), left accent border `2px solid #2563EB`, text color `#1D4ED8` font weight 600.
- **Execution Badge:** Inline pill (JetBrains Mono 10px) on the right edge showing `READY` (Slate), `RUNNING` (Blue), `PASS` (Emerald), or `FAIL` (Red).

### Selection Controls & Toggles
- **Checkboxes:** 14x14px square, 3px border radius, `1px solid #CBD5E1`, background `#FFFFFF`. Checked: background `#2563EB` with sharp white tick icon.
- **Switch Toggles:** Width 28px, height 16px, pill-styled toggle handle (12x12px). Checked background `#0D9488`.

### Viewport Image HUD & Coordinate Overlays
- **Coordinate & Zoom Strip:** Floats at the bottom edge of the image canvas. Dark slate glass or high-contrast solid surface (`#0F172A` with `#F8FAFC` mono text, opacity 90%) displaying real-time cursor values: `X: 412.5 px  Y: 188.0 px  Val: 182 (8-bit)  Zoom: 106.1%`.
- **ROI Annotations:** High-precision vectors with stroke width 1.5px non-scaling stroke, colored `#0D9488` for search areas, `#10B981` for detected features, `#EF4444` for defect anomalies.
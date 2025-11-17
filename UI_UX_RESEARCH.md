# Shotcut-Pro UI/UX Research & Design Philosophy
## Comprehensive Analysis of Modern Video Editor Interfaces (2024-2025)

**Research Date**: November 17, 2025
**Focus**: Adobe Premiere Pro, DaVinci Resolve, and Modern UI/UX Trends
**Purpose**: Guide Shotcut-Pro interface design and development

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [User Feedback Analysis](#user-feedback-analysis)
3. [UI/UX Design Trends 2024-2025](#uiux-design-trends-2024-2025)
4. [Technical Implementation](#technical-implementation)
5. [Design Philosophy Recommendations](#design-philosophy-recommendations)
6. [Shotcut-Pro UI Architecture Plan](#shotcut-pro-ui-architecture-plan)

---

## Executive Summary

### Key Findings

**What Users Want:**
- Clean, uncluttered interfaces with customizable workspaces
- Dark mode as standard (with light theme option)
- Smooth performance (60 FPS timeline scrubbing on 4K)
- Accessibility features (high contrast, large fonts, keyboard shortcuts)
- AI-powered workflow automation
- Consistency across the application

**What Users Dislike:**
- Complicated, cluttered interfaces with too many nested menus
- Performance issues and lag
- Steep learning curves without proper onboarding
- Inconsistent naming and UI patterns
- Breaking changes to familiar workflows

**Industry Standard Features:**
- Customizable workspace layouts with save/load presets
- Multiple color themes (Light, Dark, Darkest)
- Real-time preview with GPU acceleration
- Responsive timeline with smooth scrolling
- Integrated effects panels with live preview
- Professional scopes (waveform, vectorscope, histogram)

---

## User Feedback Analysis

### Adobe Premiere Pro (2024-2025)

#### What Users LIKE ✓

**Visual Design:**
- Modern appearance with Light, Dark, and Darkest themes
- Clean fonts and typography for better legibility
- Consistency with Adobe Creative Cloud ecosystem
- Rounded corners for clips (subtle but appreciated)
- Minimalist dialog boxes with clear organization

**Workflow Features:**
- **Fit and Fill** - Multi-clip selection with automatic frame fitting
- **Project Templates** - Preset workflows with predefined aspect ratios
- **Timeline Overhaul** - Live audio waveforms visible while dragging
- **New Project Dialog** - All settings in one place (name, location, templates)
- **Dynamic Waveforms** - Grow/shrink with volume adjustments

**Customization:**
- Workspace customization for different editing stages
- Sleek, customizable interface matching workflow needs
- Organized workspace with easy tool access

#### What Users DISLIKE ✗

**Performance Issues:**
- Premiere Pro 2025 described as "unusable for professional work"
- Bloated with "half-baked AI features"
- Core editing stability took "massive hit"
- Performance worse in v25 than v24 for some users
- Tracking issues causing performance degradation

**Usability Problems:**
- Not user-friendly compared to earlier versions
- Adobe changed function/effect names "for no apparent reason"
- Interface "fully worse" - settings require unwrapping layers
- Motion, scale, opacity each need separate expansion
- Described as "complicated," "bulky," and "ugly"

**Learning Curve:**
- Complex UI requires deep learning curve
- Overwhelming for new users
- Interface looks "condescing" due to feature volume
- Almost impossible to navigate without walkthrough
- Cluttered interface (though manageable with practice)

**Specific Complaints:**
- UI and performance bugs worse in Premiere Pro 25.2
- Users rolled back to 2024 version due to instability
- Steep learning curve turns away beginners

#### Requested Improvements:

**From Premiere Elements Redesign (Shared Philosophy):**
- Better accessibility in dark theme (brighter text/icon contrast)
- Workspace customization with quick switching
- Intermediate users prefer performance over new features
- Smooth workflow more important than effect quantity

---

### DaVinci Resolve (2024-2025)

#### User Preferences

**UI Structure:**
- 7 dedicated "Pages" for different workflows:
  - Media (import/organization)
  - Cut (fast editing)
  - Edit (detailed editing)
  - Fusion (VFX/motion graphics)
  - Color (color grading)
  - Fairlight (audio mixing)
  - Deliver (export)
- Fixed UI praised by professionals for consistency across studios
- Some users want more customization (Premiere-style flexibility)

**Customization Debate:**
- **Pro Fixed UI**: Consistency in all studios, easy to work anywhere
- **Pro Flexible UI**: Requested hundreds of times over 10 years
- **Requested Features**:
  - Isolate specific control palettes
  - Enlarge/shrink and reposition panels
  - Similar to older daVinci 2K system flexibility

**What Works Well:**
- Layout presets save window settings and configurations
- Page-based organization keeps workflows separate
- Clean, organized interface "seems like the future of editing"
- Professional tools grouped logically

#### Interface Comparison

**Premiere Pro vs. DaVinci Resolve:**
- Surface level: "Nearly identical" timelines, tools, and bins
- Premiere Pro: "Neat and friendly" but can feel "old and crowded"
- DaVinci Resolve: "More daunting" initially, but "clean and well-thought-out"
- Premiere Pro: Slight edge in Ease of Use (8.0 vs 7.8 on G2)
- DaVinci Resolve: Preferred by those who value organization

---

## UI/UX Design Trends 2024-2025

### 1. AI-Driven Personalization & Automation

**Trend**: Interfaces adapt to user behavior and preferences

**Implementation in Video Editors:**
- AI adjusts UI elements, menu structures, even colors
- Dynamic layout based on most-used tools
- Personalized workspace recommendations
- Auto-organizing panels based on workflow

**For Shotcut-Pro:**
- Learn user's editing patterns
- Suggest workspace layouts for detected workflow types
- Auto-hide rarely used tools
- Smart panel positioning based on usage frequency

### 2. Motion Design & Micro-Interactions

**Trend**: Subtle animations enhance usability

**Examples:**
- Smooth panel transitions
- Button hover effects
- Progress indicators with meaningful animation
- Drag-and-drop visual feedback
- Timeline clip animations (fade in/out on add/remove)

**For Shotcut-Pro:**
- Animate timeline clip movements
- Smooth zoom/pan transitions
- Visual feedback for all user actions
- Loading states for background operations

### 3. 3D Elements & Depth

**Trend**: Subtle depth creates visual hierarchy

**Application:**
- Floating panels with shadows
- Layered interface elements
- 3D preview window for spatial effects
- Depth perception in timeline (foreground/background tracks)

**For Shotcut-Pro:**
- Subtle shadows for docked panels
- Elevated effect panels
- 3D transform preview in video window
- Visual depth in multi-track timeline

### 4. Typography Emphasis

**Trend**: Big, bold, capitalized text for hierarchy

**Guidelines:**
- Large headings for sections
- Bold labels for important controls
- Clear font hierarchy (heading > subheading > body)
- Readable even at small sizes

**For Shotcut-Pro:**
- Clear panel titles (18-24pt)
- Parameter labels (12-14pt bold)
- Help text (10-12pt regular)
- Ensure readability on high-DPI displays

### 5. Accessibility as Priority

**Trend**: Accessibility is now a legal requirement (European Accessibility Act 2025)

**Requirements:**
- High contrast modes
- Screen reader support
- Keyboard-only navigation
- Adjustable text sizes
- Color-blind friendly palettes
- Focus indicators

**For Shotcut-Pro:**
- WCAG 2.1 Level AA compliance minimum
- Multiple theme options (including high contrast)
- Full keyboard navigation with visible focus
- Audio feedback for key actions (optional)
- Customizable UI scaling

### 6. Dark Mode & Visual Styles

**Trend**: Dark mode is standard, not optional

**Best Practices:**
- Dark as default (reduces eye strain)
- True black (#000000) or near-black (#1A1A1A)
- Light mode for bright environments
- Darkest mode for color grading work
- Smooth theme switching without restart

**Color Palette Recommendations:**
```
Dark Theme:
  Background: #1E1E1E
  Surface: #2D2D2D
  Border: #3E3E3E
  Text: #E0E0E0
  Accent: #4A9EFF (blue) or #48BB78 (green)

Light Theme:
  Background: #F5F5F5
  Surface: #FFFFFF
  Border: #D1D1D1
  Text: #1E1E1E
  Accent: #2563EB (blue)

Darkest Theme (for color work):
  Background: #0A0A0A
  Surface: #141414
  Border: #2A2A2A
  Text: #C0C0C0
  Accent: Muted colors to not interfere with color grading
```

### 7. Glassmorphism & Modern Effects

**Trend**: Frosted glass appearance

**Characteristics:**
- Transparent backgrounds
- Blur effects
- Subtle borders
- Light transparency

**For Shotcut-Pro:**
- Use sparingly (performance cost)
- Floating panels, modals
- Overlay controls on video preview
- Subtle blur for inactive panels

### 8. Cross-Platform Consistency

**Trend**: Unified experience across devices

**Requirements:**
- Consistent interactions (desktop, tablet, touchscreen)
- Responsive layouts
- Adaptive UI density
- Platform-specific optimizations (macOS Metal, Windows DirectX)

**For Shotcut-Pro:**
- Maintain core UX across Windows/macOS/Linux
- Respect platform conventions (menus, shortcuts)
- Optimize rendering per platform
- Touch-friendly controls where appropriate

---

## Technical Implementation

### Architecture Insights

#### Adobe Premiere Pro

**Core Technology:**
- **C++** for core application and processing
- **Multiple APIs**:
  - C++ SDK for low-level integrations
  - UXP (HTML/CSS/JavaScript) for modern plugins (v25.6+)
  - CEP (HTML5 panels) for classic extensions
  - ExtendScript for automation

**UI Implementation:**
- Core UI: Proprietary C++ framework
- Extension panels: Web technologies (HTML/CSS/JS)
- Performance: Native C++ rendering
- Plugins: Modern web stack for rapid development

**Key Takeaway**: Hybrid approach allows performance + flexibility

#### DaVinci Resolve

**Core Technology:**
- **Qt Framework** for all GUI development
- **Qt 5.x** (currently Qt 5.4.1, not yet updated to 5.6+)
- **C++** for application logic

**UI Implementation:**
- Qt Widgets for main interface
- Qt Quick potentially for modern components
- Custom platform integration (Metal on macOS, etc.)

**Architectural Decisions:**
- Fixed page-based structure
- Qt qtsingleapp for single instance
- Platform-specific rendering backends

**Key Takeaway**: Qt provides solid cross-platform foundation

#### Modern Qt for Video Editors

**Qt Capabilities:**
- Cross-platform with minimal code changes
- High-performance real-time rendering
- Perfect sync between 2D and 3D
- Event-driven architecture (efficient idle CPU/GPU)
- Native look and feel per platform

**Video-Specific Features:**
- VideoOutput for MediaPlayer/Camera content
- QML transformation support
- Shader effects for visual processing
- 60 FPS performance with optimization

**Performance Optimization:**
- High-density shaders can bottleneck
- Minimize shader effects to maintain 60 FPS
- Use Qt Quick Scene Graph optimizations
- Cache rendered frames

**Timeline UI Implementation:**
- Non-linear editing based on timeline interface
- Clips on Y-axis (tracks), time on X-axis
- Rapid prototyping needed for timeline feel
- Collapse/expand to focus on canvas
- Auto-adjust height for content

---

## Design Philosophy Recommendations

### For Shotcut-Pro

Based on comprehensive user research and competitive analysis, here are the guiding design principles:

### 1. Clarity Over Complexity

**Principle**: Every UI element should have a clear purpose

**Application:**
- Remove or hide advanced features until needed
- Use progressive disclosure (basic → advanced)
- Clear visual hierarchy (primary → secondary actions)
- Avoid nested menus deeper than 3 levels

**Example:**
```
Basic Effects Panel:
  ☑ Common (brightness, contrast, saturation)
  ☐ Advanced (curves, color wheels, LUTs)
  ☐ Creative (glitch, vintage, cinematic)
```

### 2. Performance is a Feature

**Principle**: UI must feel instantaneous

**Targets:**
- Timeline scrubbing: 60 FPS minimum
- Panel switching: <50ms
- Effect preview: <100ms initial render
- Search/filter results: <200ms

**Implementation:**
- Virtual scrolling for long lists
- Lazy loading for thumbnails
- Background thread for heavy operations
- Cancel expensive operations on user interaction

### 3. Consistency Everywhere

**Principle**: Same patterns for same actions

**Guidelines:**
- Consistent button styles and positions
- Uniform keyboard shortcuts
- Predictable panel behavior
- Standardized terminology

**Example Patterns:**
```
All "Add" actions: Plus icon in top-right
All "Remove" actions: Trash icon or X
All "Settings": Gear icon
All "Help": Question mark icon
Primary action: Blue, right-aligned
Destructive action: Red, with confirmation
```

### 4. Keyboard-First Design

**Principle**: Every action should have a keyboard shortcut

**Implementation:**
- Display shortcuts in tooltips
- Allow full customization
- Export/import shortcut profiles
- Vim-style command palette (optional)

**Essential Shortcuts:**
```
Playback:
  Space     - Play/Pause
  J/K/L     - Reverse/Pause/Forward
  I/O       - Set In/Out points

Editing:
  C         - Cut
  V         - Select tool
  B         - Blade tool
  Delete    - Ripple delete

Timeline:
  +/-       - Zoom in/out
  Z         - Fit to window
  A         - Track selection
```

### 5. Smart Defaults, Full Control

**Principle**: Works out-of-box, but experts can customize

**Approach:**
- Sensible defaults based on user research
- Easy access to preferences
- Per-project overrides
- Export/import settings profiles

**Examples:**
- Auto-save every 5 minutes (adjustable)
- 1080p timeline preview (quality vs performance slider)
- Dark theme default (3 theme options)
- 48 kHz audio (match project settings)

### 6. Contextual Help & Learning

**Principle**: Help when needed, invisible when not

**Features:**
- Inline tooltips with detailed explanations
- Video tutorials linked from UI
- Onboarding wizard for first launch
- "What's this?" mode for exploration

**Implementation:**
```qml
Button {
    text: "Export"
    tooltip: "Export timeline to video file (Ctrl+E)"
    helpUrl: "https://docs.shotcut-pro.com/export"
    onRightClick: showContextualHelp()
}
```

### 7. Feedback & Responsiveness

**Principle**: User should always know what's happening

**Visual Feedback:**
- Hover states for all interactive elements
- Active state for selected items
- Loading indicators for operations >100ms
- Progress bars with time estimates
- Success/error notifications

**Audio Feedback (Optional):**
- Confirmation beeps (can disable)
- Error sounds
- Export completion alert

### 8. Graceful Degradation

**Principle**: Works well on low-end hardware

**Strategy:**
- Detect GPU capabilities
- Offer quality vs performance slider
- Automatic proxy generation for large files
- Fallback CPU rendering
- Clear indication when GPU features disabled

**Adaptive Quality:**
```
High-End GPU:
  - Real-time 4K preview
  - GPU effects
  - Full-resolution thumbnails

Integrated Graphics:
  - 720p preview
  - CPU effects with caching
  - Lower-res thumbnails
  - Auto proxy workflow
```

---

## Shotcut-Pro UI Architecture Plan

### Overview

**Technology Stack:**
- **Qt 6.5+** (Qt Quick for UI, Qt Widgets compatibility bridge)
- **QML** for declarative UI
- **C++20** for business logic and performance-critical code
- **TypeScript** for complex QML logic (via Qt for WebAssembly tooling)

### Architecture Layers

```
┌─────────────────────────────────────────┐
│         QML UI Layer (Presentation)      │
│  - Timeline, panels, controls           │
│  - Themeable, responsive                │
└──────────────┬──────────────────────────┘
               │ Qt Meta-Object System
┌──────────────▼──────────────────────────┐
│      C++ Service Layer (Business Logic)  │
│  - VideoService, TimelineService, etc.  │
│  - State management                     │
│  - API integrations                     │
└──────────────┬──────────────────────────┘
               │ Direct calls
┌──────────────▼──────────────────────────┐
│       C++ Core Engine (Processing)      │
│  - FFmpeg integration                   │
│  - GPU rendering pipeline               │
│  - Effect processing                    │
└─────────────────────────────────────────┘
```

### Component Structure

#### 1. Main Application Window

**File**: `src/ui/MainWindow.qml`

```qml
ApplicationWindow {
    id: root

    // Theme system
    Material.theme: Settings.darkMode ? Material.Dark : Material.Light

    // Top-level layout
    ColumnLayout {
        // Menu bar
        MenuBar { /* ... */ }

        // Toolbar
        ToolBar { /* ... */ }

        // Main workspace (dockable panels)
        SplitView {
            orientation: Qt.Horizontal

            // Left sidebar (media browser, effects)
            Sidebar { width: 300 }

            // Center (timeline + preview)
            SplitView {
                orientation: Qt.Vertical
                Preview { height: parent.height * 0.6 }
                Timeline { height: parent.height * 0.4 }
            }

            // Right sidebar (properties, scopes)
            Sidebar { width: 350 }
        }

        // Status bar
        StatusBar { /* ... */ }
    }
}
```

#### 2. Timeline Component

**File**: `src/ui/components/Timeline.qml`

**Features:**
- Infinite horizontal scrolling
- Zoom with pinch/scroll
- Multi-track with drag-and-drop
- Keyframe editor integration
- Waveform visualization
- Snap to markers/clips

**Optimizations:**
- Virtual scrolling (render only visible clips)
- Canvas-based waveform drawing
- Cached thumbnails with LRU eviction
- Incremental rendering on timeline change

**Example Structure:**
```qml
Rectangle {
    id: timeline

    // Header (track names, controls)
    Row {
        Repeater {
            model: trackModel
            TrackHeader { /* ... */ }
        }
    }

    // Timeline canvas
    Flickable {
        contentWidth: timelineModel.duration * zoomFactor
        contentHeight: trackModel.count * trackHeight

        // Optimized clip rendering
        Repeater {
            model: visibleClipsModel  // Only visible clips
            Clip {
                x: model.position * zoomFactor
                width: model.duration * zoomFactor
                height: trackHeight

                // Lazy-loaded waveform
                Waveform {
                    visible: zoomFactor > 10
                    source: model.audioData
                }
            }
        }
    }

    // Playhead
    Rectangle {
        x: playheadPosition * zoomFactor
        width: 2
        height: parent.height
        color: theme.accent
    }
}
```

#### 3. Effect Panel System

**File**: `src/ui/components/EffectPanel.qml`

**Design:**
- Dynamic QML loading per effect
- Consistent parameter controls
- Real-time preview
- Keyframe toggle per parameter
- Preset save/load

**Example Effect UI:**
```qml
// Brightness effect UI
EffectPanel {
    id: brightnessEffect
    effectName: "Brightness"

    GridLayout {
        columns: 3

        Label { text: "Level" }
        Slider {
            id: levelSlider
            from: 0.0
            to: 2.0
            value: effect.getParameter("level")
            onValueChanged: effect.setParameter("level", value)
        }
        KeyframeButton {
            parameter: "level"
            enabled: timeline.currentClip != null
        }
    }
}
```

#### 4. Workspace Management

**File**: `src/ui/WorkspaceManager.qml`

**Features:**
- Save/load workspace layouts
- Preset workspaces (Editing, Color, Audio, Effects)
- Smooth transitions between workspaces
- Per-project workspace memory

**Presets:**
```
Editing Workspace:
  - Large timeline
  - Medium preview
  - Minimal sidebar (effects browser)

Color Workspace:
  - Large preview with scopes
  - Small timeline
  - Color wheels and curves panel

Audio Workspace:
  - Large timeline with waveforms
  - Audio mixer panel
  - Effects rack
```

### UI Development Workflow

#### 1. Component Development

**Process:**
1. Design mockup in Figma/Sketch
2. Create QML component skeleton
3. Implement C++ backend logic
4. Connect via Qt properties and signals
5. Add unit tests for logic
6. Test in UI with various themes
7. Optimize performance

#### 2. Iteration Cycle

**Hot Reload:**
- QML supports live editing
- Change QML file → Instant UI update
- No recompile for UI tweaks
- Fast iteration on design

**A/B Testing:**
- Feature flags for experimental UIs
- Collect usage metrics (opt-in)
- Data-driven design decisions

#### 3. Performance Profiling

**Tools:**
- Qt Creator profiler
- QML Profiler for rendering bottlenecks
- Custom metrics (frame time, memory usage)

**Targets:**
- QML creation time: <16ms (60 FPS)
- JS execution time: <5ms per frame
- Paint time: <10ms per frame

---

## Implementation Priorities

### Phase 1: Core UI Framework (Weeks 1-4)

**Goals:**
- Establish theme system
- Create basic component library
- Set up workspace management
- Implement docking system

**Deliverables:**
- Themeable application shell
- 20+ reusable QML components
- Workspace save/load functionality
- Dark/Light/Darkest themes

### Phase 2: Timeline UI (Weeks 5-8)

**Goals:**
- Professional timeline component
- Clip drag-and-drop
- Zoom and scroll optimization
- Waveform visualization

**Deliverables:**
- 60 FPS timeline scrubbing
- Multi-track editing
- Smooth zoom (0.1x to 100x)
- Visual feedback for all operations

### Phase 3: Effect & Property Panels (Weeks 9-12)

**Goals:**
- Dynamic effect UI loading
- Consistent parameter controls
- Keyframe editor integration
- Real-time preview

**Deliverables:**
- 30+ effect UIs
- Keyframe curve editor
- Preset management
- Copy/paste effect settings

### Phase 4: Polish & Accessibility (Weeks 13-16)

**Goals:**
- Keyboard navigation
- Screen reader support
- High contrast mode
- Onboarding wizard

**Deliverables:**
- WCAG 2.1 Level AA compliance
- Full keyboard control
- Contextual help system
- First-run experience

---

## Success Metrics

### Usability

- **Onboarding**: 80% of new users complete first edit in <10 minutes
- **Task Completion**: 95% of common tasks in <3 clicks
- **Learning Curve**: 70% of features discoverable without documentation

### Performance

- **Responsiveness**: 100% of UI interactions <100ms
- **Smoothness**: 60 FPS sustained during timeline scrubbing
- **Memory**: UI uses <500MB RAM

### Accessibility

- **Keyboard**: 100% of features accessible via keyboard
- **Screen Reader**: Full NVDA/VoiceOver support
- **Color Blind**: Passes Coblis color blindness simulator

### User Satisfaction

- **Net Promoter Score**: >40
- **Ease of Use**: 4.5/5 stars minimum
- **Feature Discoverability**: 4.0/5 stars minimum

---

## Conclusion

The research reveals clear patterns:

**Users Value:**
1. Performance and stability over feature quantity
2. Clean, customizable interfaces
3. Consistent design patterns
4. Powerful but not overwhelming

**Users Avoid:**
- Cluttered, inconsistent interfaces
- Performance issues
- Steep learning curves without help
- Breaking changes to workflows

**Shotcut-Pro Strategy:**
- Build on Qt/C++ for performance and flexibility
- Modern QML UI for consistency and beauty
- Progressive disclosure for feature complexity
- Extensive customization with smart defaults
- Accessibility as a core requirement

By following these principles and learning from Premiere Pro and DaVinci Resolve's successes and mistakes, Shotcut-Pro can deliver a world-class editing experience that appeals to both beginners and professionals.

---

**Document Version**: 1.0
**Created**: November 17, 2025
**Research Sources**: Adobe Premiere Pro 2024-2025, DaVinci Resolve 19-20, UI/UX Design Trends 2024-2025
**Next Update**: After Phase 1 UI implementation

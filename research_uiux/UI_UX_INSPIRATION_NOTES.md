# UI/UX Inspiration Notes

This file extracts reusable design and engineering patterns for original projects. It does not copy assets or proprietary code.

## Transferable Architecture Patterns

## 1. Wrap Legacy Flow Instead of Replacing It

Observed in:

- `CHudPause_patches.cpp`
- `CTitleStateIntro_patches.cpp`
- `CTitleStateMenu_patches.cpp`

Pattern:

- Keep the original state owner alive.
- Intercept its `Update` seam.
- Open a custom modal/submenu only when needed.
- Suppress the original update only while the custom flow is active.

Why it is valuable:

- You preserve original timing, camera, and game-state assumptions while adding modern UX

## 2. Give Every Screen the Same Tiny Lifecycle

Observed naming:

- `Init`
- `Open`
- `Draw`
- `Close`
- `CanClose`

Suggested template:

```cpp
enum class ScreenState
{
    Hidden,
    Appearing,
    Idle,
    Confirming,
    Closing
};

struct ScreenController
{
    ScreenState state = ScreenState::Hidden;
    double appearTime = 0.0;

    void init();
    void open();
    void update(double now);
    void draw();
    void close();
    bool canClose() const;
};
```

Why it is valuable:

- It makes timing, lockouts, and transitions legible across the whole UI layer

## 3. Separate Visibility From Interactivity

Observed in:

- `message_window.cpp`
- `options_menu.cpp`

Pattern:

- A screen can be visible before controls are active.
- The first confirmation press can reveal controls instead of committing an action.
- `CanClose()` can depend on both state and interaction readiness.

Suggested rule:

- Track `isVisible`, `isClosing`, and `isInteractive` independently.

## 4. Use a Shared Prompt Abstraction

Observed in:

- `button_guide.cpp`

Suggested template:

```cpp
enum class PromptSide { Left, Right };

struct ButtonPrompt
{
    ActionId action;
    PromptSide side;
    std::string label;
    bool (*visiblePredicate)() = nullptr;
};
```

Why it is valuable:

- Screen logic does not need to know which controller family or icon atlas is active

## 5. Queue Notifications Instead of Spawning Them Blindly

Observed in:

- `achievement_overlay.cpp`

Suggested template:

```cpp
struct ToastPayload
{
    int id;
    std::string title;
    std::string subtitle;
};

class ToastQueue
{
public:
    void enqueue(ToastPayload payload);
    void update(double now);
    void draw();

private:
    std::deque<ToastPayload> pending;
    bool visible = false;
    bool closing = false;
    double appearTime = 0.0;
};
```

Why it is valuable:

- Toasts stop fighting each other and respect audio/system readiness

## 6. Put Aspect-Ratio Logic Above Screen Logic

Observed in:

- `aspect_ratio_patches.cpp`
- `button_guide.cpp`
- `black_bar.cpp`

Suggested template:

```cpp
struct UiViewportTransform
{
    float referenceWidth = 1280.0f;
    float referenceHeight = 720.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float scale = 1.0f;

    Vec2 toScreen(Vec2 referencePoint) const;
};
```

Why it is valuable:

- Every screen can keep a stable authored coordinate system

## Naming Patterns Worth Reusing

- `appearTime`
- `isClosing`
- `isRestartRequired`
- `CanClose`
- `ComputeOffsets`
- `ComputeMotion`
- `DrawContainer`
- `DrawSelection*`
- `ResetSelection`

These names are useful because they communicate UI intent instead of raw implementation detail.

## Recommended Screen-State Template

For menus and modals:

1. `Hidden`
2. `Appearing`
3. `Idle`
4. `Editing` or `AwaitingChoice`
5. `Confirming` or `Cancelling`
6. `Closing`

For overlays/toasts:

1. `Queued`
2. `Appearing`
3. `Visible`
4. `Closing`
5. `Hidden`

## Recommended Input Rules

- Buffer accept input on open to prevent accidental first activation.
- Distinguish navigation input from confirm/cancel input.
- Disable close during edit mode or while a transition is mid-flight.
- Reveal button prompts only when the current UI state can actually use them.

## Recommended Layering Model

Use separate layers for:

- base scene
- host menu/screen
- modal windows
- button prompts
- transient overlays/toasts
- full-screen fades
- aspect-ratio/cinematic bars

That layering lets you add modern UX without flattening everything into one giant menu class.

## Good Engineering Heuristic

If you are adapting a legacy game UI:

- patch or wrap update seams
- centralize fades and prompts
- keep state names explicit
- make visibility, interactivity, and closure separate concerns
- keep viewport transforms global, not per-widget

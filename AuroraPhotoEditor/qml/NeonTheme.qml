pragma Singleton
import QtQuick 2.0
import Sailfish.Silica 1.0

// Design tokens for the "Neon Pop" visual style (see
// design_handoff_neon_pop_photoeditor/README.md, section "Design Tokens").
// All pixel values in the handoff are specified for the reference device
// (F+ T1100, theme_pixel_ratio = 1.25). We rescale them against the
// runtime Theme.pixelRatio so the same token values stay correct on any
// Sailfish/Aurora device instead of hardcoding px per file.
QtObject {
    id: neonTheme

    // Floor at 1.0 (never shrink below the reference px values) — on some
    // runtimes Theme.pixelRatio comes back lower than the reference device's
    // 1.25, which was collapsing header icon buttons to near-invisible sizes.
    readonly property real scale: Math.max(Theme.pixelRatio / 1.25, 1.0)

    function px(referencePixels) {
        return Math.round(referencePixels * scale)
    }

    // ---- Colors ----------------------------------------------------
    readonly property color bgBase: "#15101f"
    readonly property color bgSurface: "#1a1428"
    readonly property color bgCard: "#1f1830"
    readonly property color bgChip: "#241b38"

    readonly property color textPrimary: "#f5f1ff"
    readonly property color textSecondary: "#b7a9d9"
    readonly property color textTertiary: "#8b7aa8"
    readonly property color textHint: "#6d5c8f"

    readonly property color accentPurple: "#8b5cf6"
    readonly property color accentPink: "#ec4899"
    readonly property color accentGreen: "#22e5a0"

    readonly property color errorColor: "#e2265f"
    readonly property color successColor: "#22c583"

    // Components build their own `Gradient { GradientStop { color: NeonTheme.accentPurple } ... }`
    // from these two stops — kept here as the single source of truth for the action gradient.
    readonly property color gradientStart: accentPurple
    readonly property color gradientEnd: accentPink

    // ---- Typography --------------------------------------------------
    // Unbounded — display/headings/buttons; Manrope — body/captions.
    // TODO(assets): bundle the actual Unbounded/Manrope .ttf/.otf files
    // under qml/fonts/ and load them with FontLoader; until then these
    // family names fall back to the platform default font.
    readonly property string fontDisplay: "Unbounded"
    readonly property string fontBody: "Manrope"

    readonly property int fontSizeH1: px(42)
    readonly property int fontSizeCardTitle: px(37)
    readonly property int fontSizeButton: px(30)
    readonly property int fontSizeBodyLarge: px(28)
    readonly property int fontSizeBody: px(26)
    readonly property int fontSizeCaption: px(22)

    // Old-scale Font.Weight values only (Font.Thin/ExtraLight/Medium/
    // ExtraBold are the granular Qt 5.8+ enum and don't exist on the older
    // Qt5 this app's Aurora OS target ships — see Gradient.orientation bug).
    readonly property int fontWeightDisplay: Font.Bold
    readonly property int fontWeightBold: Font.Bold
    readonly property int fontWeightMedium: Font.DemiBold

    // ---- Spacing / radii ----------------------------------------------
    readonly property int paddingTiny: px(10)
    readonly property int paddingSmall: px(20)
    readonly property int paddingMedium: px(24)
    readonly property int paddingLarge: px(28)
    readonly property int paddingXLarge: px(32)
    readonly property int paddingXXLarge: px(40)

    readonly property int radiusSmall: px(20)
    readonly property int radiusMedium: px(28)
    readonly property int radiusLarge: px(32)
    readonly property int radiusXLarge: px(44)
    readonly property int radiusPill: 999

    readonly property int headerHeight: px(140)
    readonly property int toolPanelHeight: px(180)
    readonly property int iconButtonSize: px(52)
    readonly property int ctaHeight: px(92)
    readonly property int minTouchTarget: px(64)

    // ---- Motion ---------------------------------------------------
    readonly property int fadeDuration: 400
    readonly property int debounceStyleMs: 450
    readonly property int toastDurationMs: 3000
}

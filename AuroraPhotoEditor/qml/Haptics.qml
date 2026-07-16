pragma Singleton
import QtQuick 2.0

// Thin wrapper around org.nemomobile.ngf (the same haptics backend Silica's
// own HighlightBar.qml uses for pulldown-menu feedback). Built dynamically
// via Qt.createQmlObject — exactly like HighlightBar does — rather than a
// static `import org.nemomobile.ngf 1.0` at the top of this file, so that if
// the module is ever unavailable on a given build/device, only these two
// calls silently become no-ops instead of this whole singleton (and every
// page that imports it) failing to load.
QtObject {
    id: root

    property QtObject _press
    property QtObject _release
    property bool _ready: false

    Component.onCompleted: {
        try {
            _press = Qt.createQmlObject(
                "import org.nemomobile.ngf 1.0; NonGraphicalFeedback { event: 'feedback_press' }",
                root, "HapticsPress")
            _release = Qt.createQmlObject(
                "import org.nemomobile.ngf 1.0; NonGraphicalFeedback { event: 'feedback_release' }",
                root, "HapticsRelease")
            _ready = true
        } catch (e) {
            _press = null
            _release = null
            _ready = false
        }
    }

    function press() {
        if (_ready && _press) _press.play()
    }

    function release() {
        if (_ready && _release) _release.play()
    }
}

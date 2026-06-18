import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme
import "components"

Rectangle {
    id: root
    color: Theme.palette.background

    // Typed replica of the backend; non-null once the plugin is registered,
    // `ready` tracks the live connection.
    readonly property var backend: logos.module("lez_explorer_ui")
    property bool ready: false

    // Self-managed navigation history (browser-style back/forward). Each entry
    // is a descriptor { type, param?, query? }.
    property var history: []
    property int histIndex: -1

    // --- Navigation API (pages call these via the injected `explorer`) ---
    function sameDescriptor(a, b) {
        if (!a || !b)
            return false;
        if (a.type !== b.type)
            return false;
        if (a.type === "home")
            return true;
        if (a.type === "search")
            return a.query === b.query;
        return a.param === b.param;
    }
    function navigateTo(desc) {
        // Navigating to the page we're already on is a no-op — don't grow the
        // history (so e.g. Home-while-home doesn't wake the Back button).
        var current = root.histIndex >= 0 ? root.history[root.histIndex] : null;
        if (root.sameDescriptor(current, desc))
            return;
        var h = root.history.slice(0, root.histIndex + 1);
        h.push(desc);
        root.history = h;
        root.histIndex = h.length - 1;
        root.render();
    }
    function navigateBlock(id) { root.navigateTo({ type: "block", param: id }); }
    function navigateTx(hash) { root.navigateTo({ type: "tx", param: hash }); }
    function navigateAccount(id) { root.navigateTo({ type: "account", param: id }); }
    function showSearchResults(results, query) {
        root.navigateTo({ type: "search", param: results, query: query });
    }
    function goHome() { root.navigateTo({ type: "home" }); }
    function goBack() { if (root.histIndex > 0) { root.histIndex--; root.render(); } }
    function goForward() { if (root.histIndex < root.history.length - 1) { root.histIndex++; root.render(); } }

    function render() {
        if (root.histIndex < 0)
            return;
        var d = root.history[root.histIndex];
        var props = { explorer: root };
        var url = "pages/HomePage.qml";
        if (d.type === "block") {
            url = "pages/BlockPage.qml";
            props.blockId = d.param;
        } else if (d.type === "tx") {
            url = "pages/TransactionPage.qml";
            props.txHash = d.param;
        } else if (d.type === "account") {
            url = "pages/AccountPage.qml";
            props.accountId = d.param;
        } else if (d.type === "search") {
            url = "pages/SearchResultsPage.qml";
            props.results = d.param;
            props.query = d.query || "";
        }
        pageLoader.setSource(Qt.resolvedUrl(url), props);
        navBar.canGoBack = root.histIndex > 0;
        navBar.canGoForward = root.histIndex < root.history.length - 1;
    }

    Connections {
        target: logos
        function onViewModuleReadyChanged(moduleName, isReady) {
            if (moduleName === "lez_explorer_ui")
                root.ready = isReady && root.backend !== null;
        }
    }
    Component.onCompleted: {
        root.ready = root.backend !== null && logos.isViewModuleReady("lez_explorer_ui");
        root.goHome();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing.large
        spacing: Theme.spacing.medium

        NavigationBar {
            id: navBar
            Layout.fillWidth: true
            onBackClicked: root.goBack()
            onForwardClicked: root.goForward()
            onHomeClicked: root.goHome()
        }

        SearchBar {
            Layout.fillWidth: true
            onSearchRequested: function (query) {
                if (!root.backend || query.trim().length === 0)
                    return;
                logos.watch(root.backend.search(query), function (results) {
                    var blocks = results && results.blocks ? results.blocks : [];
                    var txs = results && results.transactions ? results.transactions : [];
                    var accts = results && results.accounts ? results.accounts : [];
                    var total = blocks.length + txs.length + accts.length;
                    if (total === 1) {
                        if (blocks.length === 1)
                            root.navigateBlock(blocks[0].blockId);
                        else if (txs.length === 1)
                            root.navigateTx(txs[0].hash);
                        else
                            root.navigateAccount(accts[0].accountId);
                    } else {
                        root.showSearchResults(results, query);
                    }
                }, function (err) {
                    root.showSearchResults({ blocks: [], transactions: [], accounts: [] }, query);
                });
            }
        }

        Loader {
            id: pageLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // Readiness overlay — covers the view until the backend connects.
    Rectangle {
        anchors.fill: parent
        visible: !root.ready
        color: Theme.palette.background

        ColumnLayout {
            anchors.centerIn: parent
            spacing: Theme.spacing.medium

            BusyIndicator {
                running: !root.ready
                Layout.alignment: Qt.AlignHCenter
            }
            LogosText {
                text: "Connecting to indexer backend…"
                color: Theme.palette.textSecondary
                font.pixelSize: Theme.typography.secondaryText
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}

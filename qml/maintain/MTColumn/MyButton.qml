import QtQuick 1.1

Rectangle {
    id: root
    property alias pressed: mymouse.pressed
    property bool hovered: false
    property alias font: text.font
    property alias text: text.text
    property alias text_color: text.color
    signal clicked
    implicitWidth: 80
    implicitHeight: 20
    radius: 6
    border.width: 2
    border.color: "#aaa"
    smooth:true
    gradient: Gradient {
        GradientStop { position: 0 ; color: {
                if(!root.enabled)
                    return "#888"
                if( root.pressed )
                    return "#ff9000"
                if( root.hovered )
                    return "#eee"
                return "#fff"
            }
        }
        GradientStop { position: 1 ; color: {
                if(!root.enabled)
                    return "#777"
                if( root.pressed )
                    return "#f07000"
                if( root.hovered )
                    return "#ddd"
                return "#eee"
            }
        }
    }
    Text{
        id: text
        text: parent.text
        anchors.centerIn: parent
        color: parent.pressed?"white":"black"
    }
    MouseArea{
        id: mymouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            root.clicked()
        }
        onEntered: {
            root.hovered = true
        }
        onExited: {
            root.hovered = false
        }
    }
}

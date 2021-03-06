import QtQuick 1.1
import "MTColumn" as MTColumn
import "MainTain.js" as MainTainJs
import "./MTColumn/Column.js" as Column
import Qtvm 1.0

Rectangle {
    id:rect_columnPage
    width: 100
    height: 62
    property Item cabinetEditItem:null
    property Item loadingMask:null
    // 1.标题栏
    Rectangle{
        id:title
        width: parent.width
        height: parent.height * 0.05
        anchors{top:parent.top;topMargin: 0}
        z:5
        border{color: "gray"; width: 1}
        MouseArea{anchors.fill: parent} //mask
        Text {
            anchors.centerIn: parent
            verticalAlignment: Text.AlignVCenter
            text: qsTr("货道管理")
            font.pixelSize: (parent.height < parent.width) ? parent.height * 0.5: parent.width * 0.1;
            font{bold: true;}
        }

        MTColumn.MyButton{
            width: parent.width * 0.2
            height:parent.height * 0.8
            anchors{
                left: parent.left
                leftMargin: 5
                verticalCenter: parent.verticalCenter
            }
            font{
                pixelSize: (height < width) ? height * 0.6 : width * 0.1;
            }
            text: "编辑该柜"
            onClicked: {

                var cabinet = listView.currentItem;
                if(cabinet){
                    var item = mtGetCabinetEditItem();
                    item.isNewCabinet = false;
                    item.cabinetNo = cabinet.cabinetNo;
                    item.colSum  = cabinet.listModel_column.count
                    item.columnPage = rect_columnPage;
                    item.visible = true;
                }
            }
        }


        MTColumn.MyButton{
            width: parent.width * 0.2
            height:parent.height * 0.8
            anchors{
                right: parent.right
                rightMargin: 5
                verticalCenter: parent.verticalCenter
            }
            font{
                pixelSize: (height < width) ? height * 0.6 : width * 0.1;
            }
            text: "新增货柜"
            onClicked: {
                var item = mtGetCabinetEditItem();
                item.isNewCabinet = true;
                item.columnPage = rect_columnPage;
                item.visible = true;

            }
        }


    }
    // 2.货道管理展示区域
    Rectangle{
        id:main_rect
        width: parent.width
        height: parent.height * 0.9
        z:3
        anchors{top:title.bottom}
        //主界面
        Rectangle {
            id: listView_rect
            anchors.fill: parent

            ListView{
                id:listView
                anchors.fill: parent
                model: listModel
                delegate:list_delegate
                snapMode: ListView.SnapToItem
                spacing: 1
                flickDeceleration: 5000  //滑动速度
                highlightMoveSpeed:2000  //滚动速度
                orientation: ListView.Horizontal
                boundsBehavior: Flickable.DragAndOvershootBounds
                focus: true
                onMovementEnded:{
                    console.log("onMovementEnded:");
                    var i = indexAt(contentX,contentY);
                    if(i != -1 && i != listView.currentIndex){
                        listView.currentIndex = i;
                    }
                }
            }
            ListModel{
                id:listModel


            }
            Component{
                id:list_delegate
                MTColumn.VMCabinet{
                    width: listView.width
                    height: listView.height
                    cabinetNo: cabinet_no
                    topParent:  top_parent
                }
            }

        }
    }

    //3. 工具栏界面
    Rectangle{
        width: parent.width
        height: parent.height * 0.05
        anchors{bottom: parent.bottom}
        z:5
        MouseArea{anchors.fill: parent} //mask
        Row{
            id:go_rect
            width: parent.width * 0.35
            height: parent.height * 0.5
            anchors.centerIn: parent
            spacing: width * 0.05
            MTColumn.MyButton{
                id: go_previous_image
                width: parent.width * 0.35
                height: parent.height
                radius: 1
                font{bold:false;pixelSize: (width < height) ? width * 0.5: height * 0.5;}
                text: "上一页"
                onClicked: {
                    listView.decrementCurrentIndex();
                }
            }
            Text {
                id: go_text
                width: parent.width * 0.2
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: (listView.currentIndex + 1) + "/" + listView.count
                font{bold:true;pixelSize: (width < height) ? width * 0.6: height * 0.6;}
                color: "blue"
            }
            MTColumn.MyButton{
                id: go_next_image
                width: parent.width * 0.35
                height: parent.height
                radius: 1
                font{bold:false;pixelSize: (width < height) ? width * 0.5: height * 0.5;}
                text: "下一页"
                onClicked: {
                    listView.incrementCurrentIndex();
                }
            }

        }
        Timer{
            id:go_timer
            interval: 3000; running: false; repeat: true
            onTriggered:{
                go_timer.stop();
                go_rect.visible = false;
            }
        }


        MTColumn.MyButton{
            width: parent.width * 0.2
            height: parent.height * 0.75
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            text: "返回"
            font{
                bold: true
                pixelSize: (height < width) ?
                               height * 0.6 : width * 0.1;
            }
            onClicked: {
                rect_columnPage.visible = false
            }
        }
    }



    function createCabinet(no){

        listModel.append({"cabinet_no":no,
                            "top_parent":rect_columnPage})
        listView.cacheBuffer += listView.width
        console.log("创建柜子 count="  + listModel.count + " cacheBuffer:" +
                    listView.cacheBuffer)

        var index = listModel.count - 1;
        listView.currentIndex = index;
        return listView.currentItem;
    }



    function flush(){
        console.log("货道列表模型");
        listModel.clear(); //清楚柜子
        for(var j = 0;j < sqlCabinetList.count;j++){
            var cab = sqlCabinetList.at(j);
            if(cab){
                var cabinet = createCabinet(cab.id);
                var columnList = cab.getColumnList();
                cabinet.listClear();
                if(columnList){
                    for(var i = 0;i < columnList.size;i++){
                        var col = columnList.at(i);
                        if(col){
                            cabinet.vmCreateColumnByColumn(col);
                        }
                    }
                }
            }
        }
    }

    function columnClear(){
        product_model.clear();
    }


    function mtGetCabinetEditItem(){
        if(cabinetEditItem == null){
            cabinetEditItem =  MainTainJs.loadComponent(rect_columnPage,"./MTColumn/MTCabinetEditPage.qml");
            cabinetEditItem.connectSignal();
        }
        return cabinetEditItem;
    }

    function loadingFinished(type,obj){
        console.log("槽:货道管理页面" + "type=" + type + " obj=" + obj);

    }

}


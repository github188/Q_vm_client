#include "vmsqlite.h"
#include <QtDebug>
#include "setting.h"

VMSqlite::VMSqlite(QObject *parent) : QObject(parent)
{
    sqlConnected = false;
    productList = new SqlProductList(this);
    cabinetList = new SqlCabinetList(this);
    orderList = new OrderList(this);//订单管理接口类
}

VMSqlite::~VMSqlite()
{
    m_db.close();
}


//启动数据库
void VMSqlite::sqlActionSlot(int type,QObject *obj)
{
    qDebug()<<trUtf8("数据库线程操作")<<" type="<<type<<" obj="<<obj;
    int mt = type;
    if(mt == SQL_START)
    {
        sqlStart();
    }
    else if(mt == SQL_COLUMN_CHECK){
        checkTableCabinet();
    }
    else if(mt == SQL_ORDER_MAKE){

    }


}






void VMSqlite::checkTableProduct()
{
    qDebug()<<"checkTableProduct";
    QSqlQuery query(m_db);
    query.exec("SELECT * FROM vmc_product");
    while(query.next()){
        bool ok;
        SqlProduct *product = new SqlProduct(0);
        product->id = query.value(1).toString();
        product->kind = query.value(2).toString();
        product->name = query.value(5).toString();
        product->salePrice = query.value(7).toUInt(&ok);
        product->pic = query.value(9).toString();
        product->imagePath = vmConfig.productImagePath() + "/"  + product->id;

        product->images = vmConfig.getFilePicList(product->imagePath);
        QFile file(product->pic);
        if(!file.exists()){
            if(product->images.isEmpty()){
                product->pic = vmConfig.productDefaultPic();
            }
            else{
                product->pic = product->imagePath + "/" + product->images[0];
            }
        }
       // qDebug()<<"VMSqlite::checkTableProduct...obj="<<product<<product->imagePath;
        productList->hashInsert(product->id,product);


    }
    productList->getProductList();
    emit sqlActionSignal(SQL_PRODUCT_ADD,(QObject *)productList);

//    SqlProduct *p = new SqlProduct();
//    for(int i = 0;i < 10000;i++){
//        p->id = QString("vm%1").arg(i,4,10,QLatin1Char('0'));
//        insertProduct(p);
//    }

}


void VMSqlite::checkTableCabinet()
{
    QSqlQuery query(m_db);

    //查询货柜表
    query.exec("SELECT * FROM vmc_cabinet");
    while(query.next()){
        bool ok;
        SqlCabinet *cabinet = new SqlCabinet();
        cabinet->id = query.value(0).toUInt(&ok);
        cabinet->sum = query.value(2).toUInt(&ok);
        cabinet->type = query.value(3).toUInt(&ok);
        cabinet->info = query.value(4).toString();
        qDebug()<<"checkTableCabinet cabinet:"<<cabinet->id;
        cabinetList->append(cabinet);
    }

    //查询货道表
    query.exec("SELECT * FROM vmc_column");
    while(query.next()){
        bool ok;
        SqlColumn *column = new SqlColumn();
        column->id = query.value(0).toUInt(&ok);
        column->bin = column->id / 1000;
        column->column = column->id % 1000;
        column->state = query.value(3).toUInt(&ok);
        column->productNo = query.value(4).toString();
        column->remain = query.value(5).toUInt(&ok);
        column->capacity = query.value(6).toUInt(&ok);

        for(int i = 0;i < cabinetList->count();i++){
            SqlCabinet *cabinet = cabinetList->at(i);
            if(cabinet->id == column->bin){ //找到对应柜号
                SqlColumnList *columnList = cabinet->columnList;
                columnList->append(column);
                break;
            }
        }
    }
    emit sqlActionSignal(SQL_CABINET_CHECK_FINISH,(QObject *)cabinetList);
}




void VMSqlite::sqlStart()
{
    if(openSqlDatabase("vmc"))
    {
        sqlConnected = true;
        emit sqlActionSignal(SQL_CONNECT_OK ,NULL);
        createTableCabinet();
        createTableProduct();
        createTableColumn();

        checkTableProduct();
        checkTableCabinet();
    }
    else
    {
        emit sqlActionSignal(SQL_CONNECT_FAIL ,NULL);
    }
}



bool VMSqlite::openSqlDatabase(const QString &fileName)
{
    if(!m_db.isOpen()){//如果数据库未打开
        m_db = QSqlDatabase::addDatabase ("QSQLITE");
        m_db.setHostName ("localhost");
        m_db.setDatabaseName (fileName + ".db");
        m_db.setUserName ("root");
        m_db.setPassword ("123456");
        return  m_db.open();
    }
    return true;
}


bool VMSqlite::tableAvailable(const QString &tableName)
{
    if(tableName!="" && m_db.isOpen ()){//如果数据库已经打开
        QString temp = "create table if not exists "+
                        tableName +
                        "(myindex INTEGER,senderUin VARCHAR[16],message TEXT,mydate DATE,mytime TIME)";
        //创建一个表，如果这表不存在，表的列为uin message mydate mytime
        QSqlQuery query = m_db.exec (temp);
        if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
            return true;
        }else{
            qDebug()<<"执行"<<temp<<"出错："<<query.lastError ().text ();
        }
    }else{
        qDebug()<<"数据库未打开";
    }
    return false;
}



bool VMSqlite::createTableProduct()
{
    QString tableName = "vmc_product";
    if(m_db.isOpen ()){//如果数据库已经打开
        QString temp = "create table if not exists "+
                tableName + " (" +
                "id integer primary key AUTOINCREMENT," +
                "productNo varchar(100)," +
                "kind varchar(100)," +
                "sellTag varchar(200)," +
                "brandName varchar(200)," +
                "productName varchar(200)," +
                "aliasName varchar(200)," +
                "salesPrice unsigned integer," +
                "productTXT TEXT," +
                "picture varchar(200)"
                        ")";// "salesPrice decimal(20,0)," +
        //创建一个表，如果这表不存在，
        QSqlQuery query = m_db.exec (temp);
        if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
            return true;
        }else{
            qDebug()<<"执行"<<temp<<"出错："<<query.lastError ().text ();
        }
    }else{
        qDebug()<<"数据库未打开";
    }
    return false;
}



bool VMSqlite::createTableCabinet()
{
    QString tableName = "vmc_cabinet";
    if(m_db.isOpen ()){//如果数据库已经打开
        QString temp = "create table if not exists "+
                        tableName + " (" +
                        "id integer primary key AUTOINCREMENT," +
                        "no     integer," +
                        "sum    integer," +
                        "type   integer," +
                        "info   TEXT" +
                        ")";
        //创建一个表，如果这表不存在，
        QSqlQuery query = m_db.exec (temp);
        if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
            return true;
        }else{
            qDebug()<<"执行"<<temp<<"出错："<<query.lastError().text ();
        }
    }else{
        qDebug()<<"数据库未打开";
    }
    return false;
}

bool VMSqlite::createTableColumn()
{
    QString tableName = "vmc_column";
    if(m_db.isOpen ()){//如果数据库已经打开
        QString temp = "create table if not exists "+
                        tableName + " (" +
                        "id integer primary key AUTOINCREMENT," +
                        "cabinetNo integer," +
                        "columnNo integer," +
                        "columnState integer," +
                        "productNo varchar(100)," +
                        "remain integer," +
                        "capacity integer," +
                        "message TEXT" +
                        ")";
        //创建一个表，如果这表不存在，
        QSqlQuery query = m_db.exec (temp);
        if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
            return true;
        }else{
            qDebug()<<"执行"<<temp<<"出错："<<query.lastError ().text ();
        }
    }else{
        qDebug()<<"数据库未打开";
    }
    return false;
}


bool VMSqlite::updateColumn(const SqlColumn *column)
{
    QString tableName = "vmc_column";
    if(!m_db.isOpen()){
        qDebug()<<"updateColumn:"<< "数据库未打开";
        return false;
    }


    QString temp = QString("update %1 set cabinetNo=%2,columnNo=%3,columnState=%4,productNo='%5',remain=%6,capacity=%7,message='%8' where id='%9'")
            .arg(tableName).arg(column->bin).arg(column->column).arg(column->state).arg(column->productNo)
            .arg(column->remain).arg(column->capacity).arg(column->message).arg(column->id);
    qDebug()<<"updateColumn:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"updateColumn:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }
}


bool VMSqlite::updateCabinet(const SqlCabinet *cabinet)
{
    QString tableName = "vmc_cabinet";
    if(!m_db.isOpen()){
        qDebug()<<"updateCabinet:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("update %1 set id=%2,no=%3,sum=%4,type=%5,info='%6'")
            .arg(tableName).arg(cabinet->id).arg(cabinet->id)
            .arg(cabinet->sum).arg(cabinet->type).arg(cabinet->info);

    qDebug()<<"updateCabinet:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"updateCabinet:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }
}

bool VMSqlite::updateProduct(const SqlProduct *product)
{
    QString tableName = "vmc_product";
    if(!m_db.isOpen()){
        qDebug()<<"updateProduct:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("update %1 set kind='%2',sellTag='%3',brandName='%4',productName='%5',aliasName='%6',salesPrice='%7',productTXT='%8',picture='%9' where productNo='%10'")
            .arg(tableName).arg(product->kind).arg(product->sellTag).arg(product->brandName)
            .arg(product->name).arg(product->aliasName).arg(product->salePrice)
            .arg(product->productTXT).arg(product->pic).arg(product->id);

    qDebug()<<"updateProduct:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"updateProduct:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }

}

bool VMSqlite::deleteColumnByCabinet(const int no)
{
    QString tableName = "vmc_column";
    if(!m_db.isOpen()){
        qDebug()<<"deleteColumn:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("delete from %1 where cabinetNo=%2")
            .arg(tableName).arg(no);

    qDebug()<<"deleteColumn:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"deleteColumn:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }
}

bool VMSqlite::deleteColumn(const SqlColumn *column)
{
    QString tableName = "vmc_column";
    if(!m_db.isOpen()){
        qDebug()<<"deleteColumn:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("delete from %1 where id=%2")
            .arg(tableName).arg(column->id);

    qDebug()<<"deleteColumn:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"deleteColumn:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }
}


bool VMSqlite::deleteCabinet(const SqlCabinet *cabinet)
{
    QString tableName = "vmc_cabinet";
    if(!m_db.isOpen()){
        qDebug()<<"deleteCabinet:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("delete from %1 where id=%2")
            .arg(tableName).arg(cabinet->id);

    qDebug()<<"deleteCabinet:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"deleteCabinet:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }
}

bool VMSqlite::deleteProduct(const QString &productNo)
{
    QString tableName = "vmc_product";
    if(!m_db.isOpen()){
        qDebug()<<"deleteProduct:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("delete from %1 where productNo='%2'")
            .arg(tableName).arg(productNo);

    qDebug()<<"deleteProduct:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"deleteProduct:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }
}

bool VMSqlite::insertProduct(const SqlProduct *product)
{
    QString tableName = "vmc_product";
    if(!m_db.isOpen()){
        qDebug()<<"insertProduct:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("insert into %1 values(NULL,'%2','%3','%4','%5','%6','%7',%8,'%9','%10')")
            .arg(tableName).arg(product->id).arg(product->kind).arg(product->sellTag)
            .arg(product->brandName).arg(product->name).arg(product->aliasName)
            .arg(product->salePrice).arg(product->productTXT).arg(product->pic);

    qDebug()<<"insertProduct:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"insertProduct:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }


}



bool VMSqlite::insertColumn(const SqlColumn *column)
{
    QString tableName = "vmc_column";
    if(!m_db.isOpen()){
        qDebug()<<"insertColumn:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("insert into %1 values(%2,%3,%4,%5,'%6',%7,%8,'%9')")
            .arg(tableName).arg(column->id).arg(column->bin)
            .arg(column->column).arg(column->state).arg(column->productNo)
            .arg(column->remain).arg(column->capacity).arg(column->message);

    qDebug()<<"insertColumn:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"insertColumn:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }
}

bool VMSqlite::insertCabinet(const SqlCabinet *cabinet)
{
    QString tableName = "vmc_cabinet";
    if(!m_db.isOpen()){
        qDebug()<<"insertCabinet:"<< "数据库未打开";
        return false;
    }

    QString temp = QString("insert into %1 values(%2,%3,%4,%5,'%6')")
            .arg(tableName).arg(cabinet->id).arg(cabinet->id)
            .arg(cabinet->sum).arg(cabinet->type).arg(cabinet->info);

    qDebug()<<"insertCabinet:"<<"temp="<<temp;
    QSqlQuery query = m_db.exec (temp);
    if(query.lastError ().type ()==QSqlError::NoError){//如果上面的语句执行没有出错
        return true;
    }else{
        qDebug()<<"insertCabinet:执行"<<temp<<"出错："<<query.lastError ().text ();
        return false;
    }
}

void VMSqlite::addOrder(const QString &productId)
{
    qDebug()<<"VMSqlite::addOrder--"<<"productId=="<<productId<<
              " orderList="<<orderList;

    QList<Order *> list = orderList->list;
    for(int i = 0;i < list.count();i++){
        Order *order = list.at(i);
        if(order && order->id == productId){
            order->buyNum++;
            return;
        }
    }



    SqlProduct *product = productList->hashValue(productId);
    if(product == NULL){
        qWarning()<<tr("VMSqlite::addOrder---product == NULL");
        return;
    }

    //新添加商品
    Order *order = new Order();
    order->buyNum = 1;
    order->id = productId;
    order->name = product->name;
    order->salePrice = product->salePrice;
    order->columnList.clear();//绑定货道
    //order->columnList = columnList->multiHash.values(productId);
    orderList->list << order;
}


bool VMSqlite::vmDeleteProduct(const QString &productId)
{
    SqlProduct *p = productList->hashValue(productId);
    if(p == NULL){
        qDebug()<<"vmInsertProduct:p=NULL";
        return false;
    }
    else{
       deleteProduct(productId);//删除数据库
       vmConfig.deleteDir(p->imagePath);//删除图片文件夹
       productList->remove(productId);//删除链表
       return true;
    }

}

bool VMSqlite::vmUpdateProduct(const QString &productId)
{
    SqlProduct *p = productList->hashValue(productId);
    if(p == NULL){
        qDebug()<<"vmInsertProduct:p=NULL";
        return false;
    }
    else{
       return updateProduct(p);
    }
}

bool VMSqlite::vmInsertProduct(const QString &productId)
{
    SqlProduct *p = productList->hashValue(productId);
    if(p == NULL){
        qDebug()<<"vmInsertProduct:p=NULL";
        return false;
    }
    else{
        return insertProduct(p);
    }
}


bool VMSqlite::vmUpdateColumn(const int id)
{
    SqlColumn *column = cabinetList->getColumn(id);
    if(column == NULL){return false;}
    bool ok = updateColumn(column);
    return ok;
}


bool VMSqlite::vmDeleteCabinet(const int no)
{
    qDebug()<<"vmDeleteCabinet:柜号="<<no;
    SqlCabinet *cabinet = cabinetList->get(no);
    if(cabinet == NULL){
        qDebug()<<"vmDeleteCabinet:该柜不存在!";
        return false;
    }
    else{
        bool ok = deleteCabinet(cabinet); //删除柜子数据库
        if(!ok){
            return false;
        }
        ok = deleteColumnByCabinet(cabinet->id);//删除该柜所有货道 数据库

        SqlColumnList *columnList = cabinet->getColumnList();
        columnList->clear(); //删除该柜所有货到链表
        cabinetList->remove(cabinet->id); //删除柜子链表
        return true;
    }
}

bool VMSqlite::vmCreateCabinet(const int no)
{
    qDebug()<<"vmCreateCabinet:柜号="<<no;
    SqlCabinet *cabinet = cabinetList->get(no);
    if(cabinet == NULL){
        qDebug()<<"vmCreateCabinet:该柜不存在!";
        return false;
    }
    else{
        bool ok = insertCabinet(cabinet);
        if(!ok){
            return false;
        }
        ok = deleteColumnByCabinet(cabinet->id);
        for(quint32 i = 0;i < cabinet->sum;i++){
            SqlColumn *column = new SqlColumn(0);
            column->bin = cabinet->id;
            column->column = i + 1;
            column->id = column->bin * 1000 + column->column;
            column->capacity = 1;
            column->remain = 0;
            column->state = VmcMainFlow::EV_COLUMN_EMPTY;
            ok = insertColumn(column);
            if(ok){
                cabinet->columnList->append(column); //插入链表
            }
            else{
                //创建失败
                delete column;
            }

        }

        return true;
    }

}




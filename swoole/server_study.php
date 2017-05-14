<?php
$server = new \swoole_server("127.0.0.1", 8088, SWOOLE_PROCESS, SWOOLE_SOCK_TCP);

$server->on('connect', function ($serv, $fd)
{
    $serv->tick(1000, function() use ($serv, $fd) {
        $serv->send($fd, "这是一条定时消息\n");
    });
});

//我们修改一下on reveive的回调，然后启动服务
$server->on('receive', function ($serv, $fd, $from_id, $data)
{
    //根据收到的消息做出不同的响应
    switch($data)
    {
        case 1:
        {
            foreach($serv->connections as $tempFD)
            {
                # 注: $tempFD 是全体client, $fd 是当前client.
                $serv->send($tempFD,"client {$fd} say : 1 for apple\n");
            }
            break;
        }
        case 2:
        {
            $serv->send($fd,"2 for boy\n");
            break;
        }
        default:
        {
            $serv->send($fd,"Others is default\n");
        }
    }
});

$server->on('close', function ($serv, $fd) {
});

// 在交互进程中放入一个数据。
$server->BaseProcess = "I'm base process.";
$server->MasterToManager = "Hello manager, I'm master1.";
$server->ManagerToWorker = "manager1.";

// 为了便于阅读，以下回调方法按照被起调的顺序组织
// 1. 首先启动Master进程
$server->on("start", function (\swoole_server $server) {
    echo "On master start." . PHP_EOL;
    // 先打印在交互进程写入的数据
    echo "server->BaseProcess = " . $server->BaseProcess . PHP_EOL;
    // 修改交互进程中写入的数据
    $server->BaseProcess = "I'm changed by master.";
    // 在Master进程中写入一些数据，以传递给Manager进程。
    $server->MasterToManager = "Hello manager, I'm master.";
});

// 2. Master进程拉起Manager进程
$server->on('ManagerStart', function (\swoole_server $server) {
    echo "On manager start." . PHP_EOL;
    // 打印，然后修改交互进程中写入的数据
    echo "server->BaseProcess = " . $server->BaseProcess . PHP_EOL;
    $server->BaseProcess = "I'm changed by manager.";
    // 打印，然后修改在Master进程中写入的数据
    echo "server->MasterToManager = " . $server->MasterToManager . PHP_EOL;
    $server->MasterToManager = "This value has changed in manager.";

    // 写入传递给Worker进程的数据
    $server->ManagerToWorker = "Hello worker, I'm manager.";
});

// 3. Manager进程拉起Worker进程
$server->on('WorkerStart', function (\swoole_server $server, $worker_id) {
    echo "Worker start" . PHP_EOL;
    // 打印在交互进程写入，然后在Master进程，又在Manager进程被修改的数据
    echo "server->BaseProcess = " . $server->BaseProcess . PHP_EOL;

    // 打印，并修改Master写入给Manager的数据
    echo "server->MasterToManager = " . $server->MasterToManager . PHP_EOL;
    $server->MasterToManager = "This value has changed in worker.";

    // 打印，并修改Manager传递给Worker进程的数据
    echo "server->ManagerToWorker = " . $server->ManagerToWorker . PHP_EOL;
    $server->ManagerToWorker = "This value is changed in worker.";
});

// 4. 正常结束Server的时候，首先结束Worker进程
$server->on('WorkerStop', function (\swoole_server $server, $worker_id) {
    echo "Worker stop" . PHP_EOL;
    // 分别打印之前的数据
    echo "server->ManagerToWorker = " . $server->ManagerToWorker . PHP_EOL;
    echo "server->MasterToManager = " . $server->MasterToManager . PHP_EOL;
    echo "server->BaseProcess = " . $server->BaseProcess . PHP_EOL;
});

// 5. 紧接着结束Manager进程
$server->on('ManagerStop', function (\swoole_server $server) {
    echo "Manager stop." . PHP_EOL;
    // 分别打印之前的数据
    echo "server->ManagerToWorker = " . $server->ManagerToWorker . PHP_EOL;
    echo "server->MasterToManager = " . $server->MasterToManager . PHP_EOL;
    echo "server->BaseProcess = " . $server->BaseProcess . PHP_EOL;
});

// 6. 最后回收Master进程
$server->on('shutdown', function (\swoole_server $server) {
    echo "Master shutdown." . PHP_EOL;
    // 分别打印之前的数据
    echo "server->ManagerToWorker = " . $server->ManagerToWorker . PHP_EOL;
    echo "server->MasterToManager = " . $server->MasterToManager . PHP_EOL;
    echo "server->BaseProcess = " . $server->BaseProcess . PHP_EOL;
});

$server->start();
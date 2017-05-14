<?php

/**
 * Created by PhpStorm.
 * User: kyrie
 * Date: 2017/2/12
 * Time: 下午1:34
 */
class BaseProcess
{
    private $process;

    private $process_list = [];
    private $process_use = []; // 当前进程是否使用中的标记
    private $min_worker_num = 3;
    private $max_worker_num = 6;

    private $current_num;

    public function __construct()
    {
        $this->process = new swoole_process(array($this, 'run'), false, 2);
        $this->process->start();

        swoole_process::wait(); // 阻塞回收子进程
    }

    public function run()
    {
        $this->current_num = $this->min_worker_num;

        // 任务进程池，3个worker进程
        for ($i = 0; $i < $this->current_num; $i++) {
            $process = new swoole_process(array($this, 'task_run'), false, 2);
            $pid = $process->start();
            $this->process_list[$pid] = $process;
            $this->process_use[$pid] = 0;
        }
        // 这里的foreach是捡起完成任务的进程，也就是空闲的进程，拿回来继续做任务，可以看出管理非常粗糙，至少sleep5秒之后才有
        // 使用swoole_event_add将管道加入到事件循环中，变为异步读取，因为read是阻塞读取
        foreach ($this->process_list as $process) {
            swoole_event_add($process->pipe, function ($pipe) use ($process) {
                $data = $process->read();
                var_dump($data);
                $this->process_use[$data] = 0;
            });
        }

        swoole_timer_tick(1000, function ($timer_id) {
            static $index = 0;
            $index = $index + 1;
            $flag = true;
            foreach ($this->process_use as $pid => $used) {
                if (0 == $used) {
                    $flag = false;
                    $this->process_use[$pid] = 1;
                    $this->process_list[$pid]->write($index . 'hello');
                    break;
                }
            }
            // 没有进程是闲的，如果进程还是小于最大值，继续新建进程
            if ($flag && $this->current_num < $this->max_worker_num) {
                $process = new swoole_process(array($this, 'task_run'), false, 2);
                $pid = $process->start();
                $this->process_list[$pid] = $process;
                $this->process_use[$pid] = 1;
                $this->process_list[$pid]->write($index . 'hello');
                $this->current_num++;
            }
            var_dump($index);
            if (10 == $index) {
                foreach ($this->process_list as $process) {
                    $process->write('exit');
                }
                swoole_timer_clear($timer_id);
                $this->process->exit();
            }
        });
    }

    public function task_run($worker)
    {
        swoole_event_add($worker->pipe, function ($pipe) use ($worker) {
            $data = $worker->read();
            var_dump($worker->pid . ':' . $data);
            if ('exit' == $data) {
                $worker->exit();
                exit;
            }
            sleep(5);
            $worker->write('' . $worker->pid);
        });
    }
}

new BaseProcess();
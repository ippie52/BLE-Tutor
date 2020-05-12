#!/usr/bin/python3
# ------------------------------------------------------------------------------
##  @file   killable_thread.py
##  @brief  Thread class that can be killed.
#
# ------------------------------------------------------------------------------
#                  Kris Dunning ippie52@gmail.com 2018.
# ------------------------------------------------------------------------------

from threading import Thread, Lock

class KillableThread(Thread):
    """
    @brief  Class to maintain a thread that can be killed upon request
    """
    def __init__(self, running=False):
        """
        @brief  Constructor - Initialises the thread and sets the running status
        @param  running - Indicates whether the thread should run immediately.
                default False.
        """
        self._running_lock = Lock()
        self._running = running
        self._has_started = False
        Thread.__init__(self)

        if running:
            self.start()

    def start(self):
        """
        @brief  Sets the running flag then starts the thread
        """
        self._running_lock.acquire(True)
        self._running = True
        self._running_lock.release()
        self._has_started = True
        Thread.start(self)

    def join(self):
        if self._has_started:
            Thread.join(self)

    def kill(self, block=False):
        """
        @brief  Kills the running thread by setting the running status to False
        @param  block - Blocks by joining the running thread until completion
        """
        self._running_lock.acquire(True)
        self._running = False
        self._running_lock.release()
        if block:
            self.join()

    def is_running(self):
        """
        @brief  Returns whether the thread should br in a running state or
                whether it has been killed
        @return True if running
        """
        self._running_lock.acquire(True)
        running = self._running
        self._running_lock.release()
        return running

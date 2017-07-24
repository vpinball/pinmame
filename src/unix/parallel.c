/*
 * This is code that, if properly implemented, can parallelize operations for
 * SMP.  Right now it's just a placeholder that does nothing.  See 
 * mess/windows/parallel.c and .h if you want inspiration.  :-)
 */
void osd_parallelize(void (*task)(void *param, int task_num, int task_count),
		void *param, int max_tasks)
{
	task(param, 0, 1);
}

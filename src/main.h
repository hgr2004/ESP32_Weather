/* ********************************************************************************************************************
 * 秉承“谁持有，谁释放”的基本原则，并且必须是先获取后释放，以下定义一个SmartLocker类，这是一个简化锁定和解锁互斥对象的方便类，
 * 实现互斥量的自动上锁和自动解锁，即构造函数实现上锁，析构函数实现解锁，如此SmartLocker的实例在离开作用域时就自动对Mutex解锁了。   
 *
********************************************************************************************************************* */
class SmartLocker
{
  //#define debugEnable
  public:
    SmartLocker(SemaphoreHandle_t* mutex, TickType_t xTicksToWait):m_Mutex(mutex)
    {
      if(m_Mutex != NULL)
      {
        isLocked = xSemaphoreTake(*m_Mutex, xTicksToWait) == pdTRUE;
        #ifdef debugEnable
        if(isLocked)
        {
          Serial.println("xSemaphoreTake is ok.");
        }
        else
        {
          Serial.println("xSemaphoreTake is error.");          
        }
        #endif
      }
    };
    ~SmartLocker()
    {
      if(isLocked)
      {
        xSemaphoreGive(*m_Mutex);
      }
    }
    bool IsLocked() const
    {
      return isLocked;
    }
  private:
    SemaphoreHandle_t* m_Mutex = NULL;
    bool isLocked = false;
};
// ————————————————
// 版权声明：本文为CSDN博主「香菇滑稽之谈」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
// 原文链接：https://blog.csdn.net/wwplh5520370/article/details/129942160
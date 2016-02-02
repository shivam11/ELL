// ParallelTransformIterator.tcc

// stl
#include <thread>

#define DEFAULT_MAX_TASKS 8

namespace utilities
{
    //
    // ParallelTransformIterator definitions
    //

    template <typename InType, typename OutType, typename Func, int MaxTasks>
    ParallelTransformIterator<InType, OutType, Func, MaxTasks>::ParallelTransformIterator(IIterator<InType>& inIter, Func transformFn) : _inIter(inIter), _transformFn(transformFn), _currentIndex(0), _endIndex(-1), _currentOutputValid(false)
    {
        // Fill the buffer with futures that are the result of calling std::async(transformFn) on inIter
        int maxTasks = MaxTasks == 0 ? std::thread::hardware_concurrency() : MaxTasks;
        if (maxTasks == 0) // if std::thread::hardware_concurrency isn't implemented, use DEFAULT_MAX_TASKS tasks (maybe this should be 1)
        {
            maxTasks = DEFAULT_MAX_TASKS;
        }

        _futures.reserve(maxTasks);
        for(int index = 0; index < maxTasks; index++)
        {
            if(!_inIter.IsValid())
            {
                break;
            }

            _futures.emplace_back(std::async(std::launch::async, _transformFn, _inIter.Get()));
            _inIter.Next();
        }
    }
    
    template <typename InType, typename OutType, typename Func, int MaxTasks>
    bool ParallelTransformIterator<InType, OutType, Func, MaxTasks>::IsValid() const
    {
        return _currentIndex != _endIndex;
    }

    template <typename InType, typename OutType, typename Func, int MaxTasks>
    void ParallelTransformIterator<InType, OutType, Func, MaxTasks>::Next()
    {
        if(!IsValid())
        {
            return;
        }
        _currentOutputValid = false;
        
        // If necessary, create new std::future to handle next input
        if(_inIter.IsValid())
        {
            _futures[_currentIndex] = std::async(std::launch::async, _transformFn, _inIter.Get());
            _inIter.Next();
        }
        else
        {
            if(_endIndex < 0) // Check if we've already noted the end index
            {
                _endIndex = _currentIndex;
            }
        }
        _currentIndex = (_currentIndex+1)%_futures.size();
    };
    
    template <typename InType, typename OutType, typename Func, int MaxTasks>
    OutType ParallelTransformIterator<InType, OutType, Func, MaxTasks>::Get()
    {
        // Need to cache output of current std::future, because calling std::future::get() twice is an error
        if(!_currentOutputValid)
        {
            _currentOutput = _futures[_currentIndex].get();
            _currentOutputValid = true;
        }

        return _currentOutput;
    }
}
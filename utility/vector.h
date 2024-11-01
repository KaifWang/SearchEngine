// University of Michigan, EECS 440
// Xiao Song (xiaosx@umich.edu)
//
// vector.h
// Implement STL::vector
#ifndef _UMSE_VECTOR_H
#define _UMSE_VECTOR_H

//#define _UMSE_STL_VECTOR_LOG

#include <iostream>
#include <initializer_list> // initialize list initialization
#include <cstring>          // memmove

using std::cout;
using std::endl;
using std::initializer_list;

// Traits Extractor
struct _trueType {};
struct _falseType {};

template <class T>
struct _typeTraits
   {
   typedef _trueType dummy_placeholder;
   typedef _falseType is_buildin_type;
   };

template<> struct _typeTraits<int>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<bool>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<char>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<signed char>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<unsigned char>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<short>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<unsigned short>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<unsigned int>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<long>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<float>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<double>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<char*>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<signed char*>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<unsigned char*>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<const char*>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<const signed char*>
   {
   typedef _trueType is_buildin_type;
   };

template<> struct _typeTraits<const unsigned char*>
   {
   typedef _trueType is_buildin_type;
   };

template <class T>
inline T* value_type ( const T* )
   {
   return ( T* ) ( 0 );
   }

// Destructor
template <class T>
inline void destroy ( T* ptr )
   {
   ptr->~T();
   }

template <class ForwardIter>
inline void _destroy_aux ( ForwardIter begin, ForwardIter end, _falseType )
   {
   for ( ; begin != end; begin++ )
      destroy ( &*begin );
   }

template <class ForwardIter>
inline void __destroy_aux ( ForwardIter, ForwardIter, _trueType ) {}

template <class ForwardIter, class T>
inline void _destroy ( ForwardIter begin, ForwardIter end, T* )
   {
   typedef typename _typeTraits<T>::is_buildin_type is_buildin_type;
   _destroy_aux ( begin, end, is_buildin_type() );
   }

template <class ForwardIter>
inline void destroy ( ForwardIter begin, ForwardIter end )
   {
   _destroy ( begin, end, value_type ( begin ) );
   }

inline void destroy ( char*, char* ) {}
inline void destroy ( int*, int* ) {}
inline void destroy ( long*, long* ) {}
inline void destroy ( double*, double* ) {}
inline void destroy ( float*, float* ) {}

// Constructor
template <class T>
inline void construct ( T* ptr )
   {
   new ( ptr ) T();
   }

template <class TCls, class TVal>
inline void construct ( TCls* ptr, const TVal& val )
   {
   new ( ptr ) TCls ( val );
   }

// uninitialized_copy
/* equivalent to
template<class InputIterator, class ForwardIterator>
  ForwardIterator uninitialized_copy ( InputIterator first, InputIterator last,
                                       ForwardIterator result )
{
  for (; first!=last; ++result, ++first)
    new (static_cast<void*>(&*result))
      typename iterator_traits<ForwardIterator>::value_type(*first);
  return result;
}
*/

template <class SourceIter, class TargetForwardIter>
inline TargetForwardIter _uninitialized_copy_aux ( SourceIter begin,
      SourceIter end, TargetForwardIter result, _trueType )
   {
   for ( ; begin != end; begin++, result++ )
      *result = *begin;
   return result;
   }

template <class SourceIter, class TargetForwardIter>
inline TargetForwardIter _uninitialized_copy_aux ( SourceIter begin,
      SourceIter end, TargetForwardIter result, _falseType )
   {
   for ( ; begin != end; begin++, result++ )
      construct ( &*result, *begin );
   return result;
   }

template <class SourceIter, class TargetForwardIter, class T>
inline TargetForwardIter _uninitialized_copy ( SourceIter begin, SourceIter end,
      TargetForwardIter result, T* )
   {
   typedef typename _typeTraits<T>::is_buildin_type is_build_in_type;
   return _uninitialized_copy_aux ( begin, end, result, is_build_in_type() );
   }

template <class SourceIter, class TargetForwardIter>
inline TargetForwardIter uninitialized_copy ( SourceIter begin, SourceIter end,
      TargetForwardIter result )
   {
   return _uninitialized_copy ( begin, end, result, value_type ( result ) );
   }

inline char* uninitialized_copy ( const char* begin, const char* end,
      char* result )
   {
   memmove ( result, begin, end - begin );
   return result + ( end - begin );
   }


// _uninitialized_fill_n
/* equivalent to
template < class ForwardIterator, class Size, class T >
  ForwardIterator uninitialized_fill_n ( ForwardIterator first, Size n, const T& x )
{
  for (; n--; ++first)
    new (static_cast<void*>(&*first))
      typename iterator_traits<ForwardIterator>::value_type(x);
  return first;
}
*/

template <class TargetForwardIter, class Size, class T>
inline TargetForwardIter _uninitialized_fill_n_aux ( TargetForwardIter result,
      Size n, const T& val, _trueType )
   {
   for ( ; n > 0; --n, ++result )
      *result = val;
   return result;
   }

template <class TargetForwardIter, class Size, class T>
inline TargetForwardIter _uninitialized_fill_n_aux ( TargetForwardIter result,
      Size n, const T& val, _falseType )
   {
   for ( ; n > 0; --n, ++result )
      construct ( &*result, val );
   return result;
   }

template <class TargetForwardIter, class Size, class T, class T1>
inline TargetForwardIter _uninitialized_fill_n ( TargetForwardIter result,
      Size n, const T& val, T1* )
   {
   typedef typename _typeTraits<T1>::is_buildin_type is_build_in_type;

   return _uninitialized_fill_n_aux ( result, n, val, is_build_in_type() );
   }

template <class TargetForwardIter, class Size, class T>
inline TargetForwardIter uninitialized_fill_n ( TargetForwardIter result,
      Size n, const T& val )
   {
   return _uninitialized_fill_n ( result, n, val, value_type ( result ) );
   }


template <class T>
class vector
   {
   public:
      typedef T valueType;
      typedef T* pointer;
      typedef T* iterator;
      typedef const T* constPointer;
      typedef const T* constIterator;
      typedef T& reference;
      typedef const T& constReference;
      typedef size_t sizeType;

      // Memory Management
   private:
      iterator _memStart;        // data start
      iterator _memFinish;       // one pass data end
      iterator _memEndOfStorage; // storage end

      inline iterator _memAllocate ( sizeType n )
         {
         // operator new only in charge of allocating memory
         // does not handle data initialization
         // this is C++ equivalent to malloc
         return ( iterator ) ( ::operator new ( ( size_t ) ( n * sizeof ( T ) ) ) );
         }

      inline void _memDeallocate ( iterator ArrayOfObj, sizeType n )
         {
         // operator delete only in charge of deallocate memory
         // should call destructor before calling this function
         if ( ArrayOfObj )
            ::operator delete ( ArrayOfObj );
         }

      // Allocate uninitialized storage
      inline void _memAllocateBlock ( sizeType len )
         {
         if ( len <= maxSize() )
            {
            _memStart = _memAllocate ( len );
            _memFinish = _memStart;
            _memEndOfStorage = _memStart + len;
            }
         else
            _throwLengthError();
         }

      // Deallocate storage
      inline void _memDeallocateBlock()
         {
         _memDeallocate ( _memStart, _memEndOfStorage - _memStart );
         }

      // Error
   private:
      // Index out of range
      inline void _throwOutOfRangeError() const
         {
         throw std::out_of_range ( "vector out of range error\n" );
         }

      // Allocate too much memory
      inline void _throwLengthError() const
         {
         throw std::runtime_error ( "vector length error\n" );
         }

      // Constructor Helper
   private:
      inline void _ctrRange ( constIterator sourceStart, constIterator sourceEnd )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ void _ctrRange(constIterator sourceStart, constIterator sourceEnd)\n" );
         #endif

         // Allocate Memory
         //differenceType count = sourceEnd - sourceStart;
         sizeType count = sourceEnd - sourceStart;
         _memAllocateBlock ( count );
         // Copy Content
         _memFinish = uninitialized_copy ( sourceStart, sourceEnd, _memStart );
         }

      inline void _ctrFill ( sizeType count, constReference& val )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ void _ctrFill(sizeType count, valueType val)\n" );
         #endif

         // Allocate Memory
         _memAllocateBlock ( count );
         // Construct Content
         _memFinish = uninitialized_fill_n ( _memStart, count, val );
         }

   public:
      // Default Constructor
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Constructs an empty vector with capacity 0
      vector() noexcept : _memStart ( nullptr ), _memFinish ( nullptr ),
         _memEndOfStorage ( nullptr )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector()\n" );
         #endif
         }

      // Destructor
      // REQUIRES: Nothing
      // MODIFIES: destroys *this
      // EFFECTS: Performs any neccessary clean up operations
      ~vector()
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ ~vector() on size = %zu\n", size() );
         #endif

         destroy ( _memStart, _memFinish );
         _memDeallocateBlock();
         _memStart = _memFinish = _memEndOfStorage = nullptr;
         }

      // Resize Constructor
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Constructs a vector with size num_elements,
      //    all default constructed
      //vector ( size_t num_elements )
      // NOTE: use explicit to aoivd default type conversion
      explicit vector ( sizeType count ) : _memStart ( nullptr ),
         _memFinish ( nullptr ), _memEndOfStorage ( nullptr )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ explicit vector(sizeType num_elements)\n" );
         #endif

         _ctrFill ( count, valueType() );
         }

      // Fill Constructor
      // REQUIRES: Capacity > 0
      // MODIFIES: *this
      // EFFECTS: Creates a vector with size num_elements, all assigned to val
      //vector ( size_t num_elements, const T& val )
      vector ( sizeType count, constReference val ) : _memStart ( nullptr ),
         _memFinish ( nullptr ), _memEndOfStorage ( nullptr )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector(sizeType num_elements, constReference val)\n" );
         #endif

         _ctrFill ( count, val );
         }

      // Copy Constructor
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Creates a clone of the vector other
      //vector ( const vector<T>& other )
      vector ( const vector<valueType>& other ) : _memStart ( nullptr ),
         _memFinish ( nullptr ), _memEndOfStorage ( nullptr )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector(const vector<valueType> &other)\n" );
         #endif

         _ctrRange ( other.begin(), other.end() );
         }

      vector ( constPointer begin, constPointer end ) : _memStart ( nullptr ),
         _memFinish ( nullptr ), _memEndOfStorage ( nullptr )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector(constPointer source, constPointer end)\n" );
         #endif

         _ctrRange ( begin, end );
         }

      // Assignment operator
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Duplicates the state of other to *this
      vector& operator= ( const vector<T>& rhs )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector operator=(const vector<T> &other)\n" );
         #endif

         if ( this != &rhs )
            {
            #ifdef _UMSE_STL_VECTOR_LOG
            printf ( "@ ctr new obj\n" );
            #endif

            vector tmp ( rhs );
            tmp.reserve( rhs.capacity() );

            #ifdef _UMSE_STL_VECTOR_LOG
            printf ( "@ swap\n" );
            #endif

            swap ( tmp );
            }
         return *this;
         }

      // Move Constructor
      // REQUIRES: Nothing
      // MODIFIES: *this, leaves other in a default constructed state
      // EFFECTS: Takes the data from other into a newly constructed vector
      vector ( vector<T>&& other ) noexcept : _memStart ( other._memStart ),
         _memFinish ( other._memFinish ), _memEndOfStorage ( other._memEndOfStorage )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector(vector<T> &&other)\n" );
         #endif

         other._memStart = other._memFinish = other._memEndOfStorage = nullptr;
         }

      // Move Assignment Operator
      // REQUIRES: Nothing
      // MODIFIES: *this, leaves otherin a default constructed state
      // EFFECTS: Takes the data from other in constant time
      vector& operator= ( vector<T>&& other ) noexcept
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector operator=(vector<T> &&other)\n" );
         #endif

         destroy ( _memStart, _memFinish );
         _memDeallocateBlock();

         _memStart = other._memStart;
         _memFinish = other._memFinish;
         _memEndOfStorage = other._memEndOfStorage;

         other._memEndOfStorage = other._memStart = other._memFinish = nullptr;
         return *this;
         }

      // Initialize list
      vector( initializer_list<valueType> ilist )
      {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector( initializer_list<valueType> ilist )\n" );
         #endif

         _ctrRange(ilist.begin(), ilist.end());
      }

      // Initialize list
      vector& operator= ( initializer_list<valueType> ilist )
      {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ vector& operator= ( initializer_list<valueType> ilist )\n" );
         #endif

         vector tmp(ilist.begin(), ilist.end());
         swap(tmp);
         return *this;
      }

      // Vector information
   public:
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns the number of elements in the vector
      //size_t size() const
      sizeType size() const noexcept
         {
         return sizeType ( _memFinish - _memStart );
         }

      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns the maximum size the vector can attain before resizing
      //size_t capacity() const
      sizeType capacity() const noexcept
         {
         return sizeType ( _memEndOfStorage - _memStart );
         }

      // CUSTOMIZE API
      bool empty() const noexcept
         {
         return _memFinish == _memStart;
         }

      sizeType maxSize() const
         {
         //return ( SIZE_MAX / sizeof ( valueType ) ) - 1;
         return ( sizeType ( -1 ) / sizeof ( valueType ) ) - 1;
         }

      // Access data
   public:
      // REQUIRES: 0 <= i < size()
      // MODIFIES: Allows modification of data[i]
      // EFFECTS: Returns a mutable reference to the i'th element
      //T& operator[ ] ( size_t i )
      reference operator[] ( sizeType i )
         {
         return * ( _memStart + i );
         }

      // REQUIRES: 0 <= i < size()
      // MODIFIES: Nothing
      // EFFECTS: Get a const reference to the ith element
      //const T& operator[ ] ( size_t i ) const
      constReference operator[] ( sizeType i ) const
         {
         // function return copy of iterator and convert it to const
         return * ( _memStart + i );
         }

      reference at ( sizeType i )
         {
         if ( i >= size() )
            _throwOutOfRangeError();
         return * ( _memStart + i );
         }

      constReference at ( sizeType i ) const
         {
         if ( i >= size() )
            _throwOutOfRangeError();
         return * ( _memStart + i );
         }

      // Modification
   public:
      // REQUIRES: new_capacity > capacity()
      // MODIFIES: capacity()
      // EFFECTS: Ensures that the vector can contain size() = new_capacity
      //    elements before having to reallocate
      //void reserve ( size_t newCapacity )
      void reserve ( sizeType newCapacity )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ void reserve(sizeType newCapacity) old capacity %zu, new capacity %zu\n",
               capacity(), newCapacity );
         #endif

         if ( newCapacity > maxSize() )
            _throwLengthError();

         if ( capacity() < newCapacity )
            {
            iterator _memStart_new = _memAllocate ( newCapacity );
            iterator _memFinish_new = uninitialized_copy ( _memStart, _memFinish,
                        _memStart_new );

            destroy ( _memStart, _memFinish );
            _memDeallocateBlock();

            _memStart = _memStart_new;
            _memFinish = _memFinish_new;
            _memEndOfStorage = _memStart + newCapacity;
            }
         }

      // REQUIRES: Nothing
      // MODIFIES: this, size(), capacity()
      // EFFECTS: Appends the element x to the vector, allocating
      //    additional space if neccesary
      //void pushBack ( const T& x )
      void push_back ( constReference val )
         {
         pushBack ( val );
         }

      void pushBack ( constReference val )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ void pushBack(constReference val), curr size %zu, curr capacity %zu",
               size(), capacity() );
         #endif

         if ( _memFinish == _memEndOfStorage )
            {
            #ifdef _UMSE_STL_VECTOR_LOG
            printf ( "not reach capacity, add directly\n" );
            #endif

            reserve ( ( size() == 0 ? 1 : size() * 4 ) );
            }

         construct ( _memFinish, val );
         ++_memFinish;
         }

      // REQUIRES: Nothing
      // MODIFIES: this, size()
      // EFFECTS: Removes the last element of the vector,
      //    leaving capacity unchanged
      void pop_back()
         {
         popBack();
         }

      void popBack()
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ void popBack()\n" );
         #endif

         _memFinish--;
         destroy ( _memFinish );
         }

      // Iterators
   public:
      // REQUIRES: Nothing
      // MODIFIES: Allows mutable access to the vector's contents
      // EFFECTS: Returns a mutable random access iterator to the
      //    first element of the vector
      //T* begin ( )
      iterator begin() noexcept
         {
         return _memStart;
         }

      // REQUIRES: Nothing
      // MODIFIES: Allows mutable access to the vector's contents
      // EFFECTS: Returns a mutable random access iterator to
      //    one past the last valid element of the vector
      //T* end ( )
      iterator end() noexcept
         {
         return _memFinish;
         }

      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns a random access iterator to the first element of the vector
      //const T* begin ( ) const
      constIterator begin() const noexcept
         {
         // function return copy of iterator and convert it to const
         return _memStart;
         }

      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns a random access iterator to
      //    one past the last valid element of the vector
      //const T* end ( ) const
      constIterator end() const noexcept
         {
         return _memFinish;
         }

      void swap ( vector<T>& other )
         {
         #ifdef _UMSE_STL_VECTOR_LOG
         printf ( "@ void swap ( const vector<T>& other )\n" );
         #endif

         if ( this != &other )
            {
            std::swap( _memStart, other._memStart );
            std::swap( _memFinish, other._memFinish );
            std::swap( _memEndOfStorage, other._memEndOfStorage );
            }
         }
   };

#endif
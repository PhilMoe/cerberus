
#include "main.h"

//${CONFIG_BEGIN}
#define CFG_BRL_OS_IMPLEMENTED 1
#define CFG_CD 
#define CFG_CONFIG release
#define CFG_CPP_DOUBLE_PRECISION_FLOATS 1
#define CFG_CPP_GC_MODE 0
#define CFG_HOST winnt
#define CFG_LANG cpp
#define CFG_MODPATH 
#define CFG_RELEASE 1
#define CFG_SAFEMODE 0
#define CFG_TARGET stdcpp
//${CONFIG_END}

//${TRANSCODE_BEGIN}

#include <wctype.h>
#include <locale.h>

// C++ Cerberus runtime.
//
// Placed into the public domain 24/02/2011.
// No warranty implied; use at your own risk.

//***** Cerberus Types *****

typedef wchar_t Char;
template<class T> class Array;
class String;
class Object;

#if CFG_CPP_DOUBLE_PRECISION_FLOATS
typedef double Float;
#define FLOAT(X) X
#else
typedef float Float;
#define FLOAT(X) X##f
#endif

void dbg_error( const char *p );

#if !_MSC_VER
#define sprintf_s sprintf
#define sscanf_s sscanf
#endif

//***** GC Config *****

#if CFG_CPP_GC_DEBUG
#define DEBUG_GC 1
#else
#define DEBUG_GC 0
#endif

// GC mode:
//
// 0 = disabled
// 1 = Incremental GC every OnWhatever
// 2 = Incremental GC every allocation
//
#ifndef CFG_CPP_GC_MODE
#define CFG_CPP_GC_MODE 1
#endif

//How many bytes alloced to trigger GC
//
#ifndef CFG_CPP_GC_TRIGGER
#define CFG_CPP_GC_TRIGGER 8*1024*1024
#endif

//GC_MODE 2 needs to track locals on a stack - this may need to be bumped if your app uses a LOT of locals, eg: is heavily recursive...
//
#ifndef CFG_CPP_GC_MAX_LOCALS
#define CFG_CPP_GC_MAX_LOCALS 8192
#endif

// ***** GC *****

#if _WIN32

int gc_micros(){
	static int f;
	static LARGE_INTEGER pcf;
	if( !f ){
		if( QueryPerformanceFrequency( &pcf ) && pcf.QuadPart>=1000000L ){
			pcf.QuadPart/=1000000L;
			f=1;
		}else{
			f=-1;
		}
	}
	if( f>0 ){
		LARGE_INTEGER pc;
		if( QueryPerformanceCounter( &pc ) ) return pc.QuadPart/pcf.QuadPart;
		f=-1;
	}
	return 0;// timeGetTime()*1000;
}

#elif __APPLE__

#include <mach/mach_time.h>

int gc_micros(){
	static int f;
	static mach_timebase_info_data_t timeInfo;
	if( !f ){
		mach_timebase_info( &timeInfo );
		timeInfo.denom*=1000L;
		f=1;
	}
	return mach_absolute_time()*timeInfo.numer/timeInfo.denom;
}

#else

int gc_micros(){
	return 0;
}

#endif

#define gc_mark_roots gc_mark

void gc_mark_roots();

struct gc_object;

gc_object *gc_object_alloc( int size );
void gc_object_free( gc_object *p );

struct gc_object{
	gc_object *succ;
	gc_object *pred;
	int flags;
	
	virtual ~gc_object(){
	}
	
	virtual void mark(){
	}
	
	void *operator new( size_t size ){
		return gc_object_alloc( size );
	}
	
	void operator delete( void *p ){
		gc_object_free( (gc_object*)p );
	}
};

gc_object gc_free_list;
gc_object gc_marked_list;
gc_object gc_unmarked_list;
gc_object gc_queued_list;	//doesn't really need to be doubly linked...

int gc_free_bytes;
int gc_marked_bytes;
int gc_alloced_bytes;
int gc_max_alloced_bytes;
int gc_new_bytes;
int gc_markbit=1;

gc_object *gc_cache[8];

void gc_collect_all();
void gc_mark_queued( int n );

#define GC_CLEAR_LIST( LIST ) ((LIST).succ=(LIST).pred=&(LIST))

#define GC_LIST_IS_EMPTY( LIST ) ((LIST).succ==&(LIST))

#define GC_REMOVE_NODE( NODE ){\
(NODE)->pred->succ=(NODE)->succ;\
(NODE)->succ->pred=(NODE)->pred;}

#define GC_INSERT_NODE( NODE,SUCC ){\
(NODE)->pred=(SUCC)->pred;\
(NODE)->succ=(SUCC);\
(SUCC)->pred->succ=(NODE);\
(SUCC)->pred=(NODE);}

void gc_init1(){
	GC_CLEAR_LIST( gc_free_list );
	GC_CLEAR_LIST( gc_marked_list );
	GC_CLEAR_LIST( gc_unmarked_list);
	GC_CLEAR_LIST( gc_queued_list );
}

void gc_init2(){
	gc_mark_roots();
}

#if CFG_CPP_GC_MODE==2

int gc_ctor_nest;
gc_object *gc_locals[CFG_CPP_GC_MAX_LOCALS],**gc_locals_sp=gc_locals;

struct gc_ctor{
	gc_ctor(){ ++gc_ctor_nest; }
	~gc_ctor(){ --gc_ctor_nest; }
};

struct gc_enter{
	gc_object **sp;
	gc_enter():sp(gc_locals_sp){
	}
	~gc_enter(){
#if DEBUG_GC
		static int max_locals;
		int n=gc_locals_sp-gc_locals;
		if( n>max_locals ){
			max_locals=n;
			printf( "max_locals=%i\n",n );
		}
#endif		
		gc_locals_sp=sp;
	}
};

#define GC_CTOR gc_ctor _c;
#define GC_ENTER gc_enter _e;

#else

struct gc_ctor{
};
struct gc_enter{
};

#define GC_CTOR
#define GC_ENTER

#endif

//Can be modified off thread!
static volatile int gc_ext_new_bytes;

#if _MSC_VER
#define atomic_add(P,V) InterlockedExchangeAdd((volatile unsigned int*)P,V)			//(*(P)+=(V))
#define atomic_sub(P,V) InterlockedExchangeSubtract((volatile unsigned int*)P,V)	//(*(P)-=(V))
#else
#define atomic_add(P,V) __sync_fetch_and_add(P,V)
#define atomic_sub(P,V) __sync_fetch_and_sub(P,V)
#endif

//Careful! May be called off thread!
//
void gc_ext_malloced( int size ){
	atomic_add( &gc_ext_new_bytes,size );
}

void gc_object_free( gc_object *p ){

	int size=p->flags & ~7;
	gc_free_bytes-=size;
	
	if( size<64 ){
		p->succ=gc_cache[size>>3];
		gc_cache[size>>3]=p;
	}else{
		free( p );
	}
}

void gc_flush_free( int size ){

	int t=gc_free_bytes-size;
	if( t<0 ) t=0;
	
	//ignore bytes freed by released strings
	int new_bytes=gc_new_bytes;
	
	while( gc_free_bytes>t ){
	
		gc_object *p=gc_free_list.succ;

		GC_REMOVE_NODE( p );

#if DEBUG_GC
//		printf( "deleting @%p\n",p );fflush( stdout );
//		p->flags|=4;
//		continue;
#endif
		delete p;
	}
	
	gc_new_bytes=new_bytes;
}

gc_object *gc_object_alloc( int size ){

	size=(size+7)&~7;
	
	gc_new_bytes+=size;
	
#if CFG_CPP_GC_MODE==2

	if( !gc_ctor_nest ){

#if DEBUG_GC
		int ms=gc_micros();
#endif
		if( gc_new_bytes+gc_ext_new_bytes>(CFG_CPP_GC_TRIGGER) ){
			atomic_sub( &gc_ext_new_bytes,gc_ext_new_bytes );
			gc_collect_all();
			gc_new_bytes=0;
		}else{
			gc_mark_queued( (long long)(gc_new_bytes)*(gc_alloced_bytes-gc_new_bytes)/(CFG_CPP_GC_TRIGGER)+gc_new_bytes );
		}

#if DEBUG_GC
		ms=gc_micros()-ms;
		if( ms>=100 ) {printf( "gc time:%i\n",ms );fflush( stdout );}
#endif
	}
	
#endif

	gc_flush_free( size );

	gc_object *p;
	if( size<64 && (p=gc_cache[size>>3]) ){
		gc_cache[size>>3]=p->succ;
	}else{
		p=(gc_object*)malloc( size );
	}
	
	p->flags=size|gc_markbit;
	GC_INSERT_NODE( p,&gc_unmarked_list );

	gc_alloced_bytes+=size;
	if( gc_alloced_bytes>gc_max_alloced_bytes ) gc_max_alloced_bytes=gc_alloced_bytes;
	
#if CFG_CPP_GC_MODE==2
	*gc_locals_sp++=p;
#endif

	return p;
}

#if DEBUG_GC

template<class T> gc_object *to_gc_object( T *t ){
	gc_object *p=dynamic_cast<gc_object*>(t);
	if( p && (p->flags & 4) ){
		printf( "gc error : object already deleted @%p\n",p );fflush( stdout );
		exit(-1);
	}
	return p;
}

#else

#define to_gc_object(t) dynamic_cast<gc_object*>(t)

#endif

template<class T> T *gc_retain( T *t ){
#if CFG_CPP_GC_MODE==2
	*gc_locals_sp++=to_gc_object( t );
#endif
	return t;
}

template<class T> void gc_mark( T *t ){

	gc_object *p=to_gc_object( t );
	
	if( p && (p->flags & 3)==gc_markbit ){
		p->flags^=1;
		GC_REMOVE_NODE( p );
		GC_INSERT_NODE( p,&gc_marked_list );
		gc_marked_bytes+=(p->flags & ~7);
		p->mark();
	}
}

template<class T> void gc_mark_q( T *t ){

	gc_object *p=to_gc_object( t );
	
	if( p && (p->flags & 3)==gc_markbit ){
		p->flags^=1;
		GC_REMOVE_NODE( p );
		GC_INSERT_NODE( p,&gc_queued_list );
	}
}

template<class T,class V> void gc_assign( T *&lhs,V *rhs ){

	gc_object *p=to_gc_object( rhs );
	
	if( p && (p->flags & 3)==gc_markbit ){
		p->flags^=1;
		GC_REMOVE_NODE( p );
		GC_INSERT_NODE( p,&gc_queued_list );
	}
	lhs=rhs;
}

void gc_mark_locals(){

#if CFG_CPP_GC_MODE==2
	for( gc_object **pp=gc_locals;pp!=gc_locals_sp;++pp ){
		gc_object *p=*pp;
		if( p && (p->flags & 3)==gc_markbit ){
			p->flags^=1;
			GC_REMOVE_NODE( p );
			GC_INSERT_NODE( p,&gc_marked_list );
			gc_marked_bytes+=(p->flags & ~7);
			p->mark();
		}
	}
#endif	
}

void gc_mark_queued( int n ){
	while( gc_marked_bytes<n && !GC_LIST_IS_EMPTY( gc_queued_list ) ){
		gc_object *p=gc_queued_list.succ;
		GC_REMOVE_NODE( p );
		GC_INSERT_NODE( p,&gc_marked_list );
		gc_marked_bytes+=(p->flags & ~7);
		p->mark();
	}
}

void gc_validate_list( gc_object &list,const char *msg ){
	gc_object *node=list.succ;
	while( node ){
		if( node==&list ) return;
		if( !node->pred ) break;
		if( node->pred->succ!=node ) break;
		node=node->succ;
	}
	if( msg ){
		puts( msg );fflush( stdout );
	}
	puts( "LIST ERROR!" );
	exit(-1);
}

//returns reclaimed bytes
void gc_sweep(){

	int reclaimed_bytes=gc_alloced_bytes-gc_marked_bytes;
	
	if( reclaimed_bytes ){
	
		//append unmarked list to end of free list
		gc_object *head=gc_unmarked_list.succ;
		gc_object *tail=gc_unmarked_list.pred;
		gc_object *succ=&gc_free_list;
		gc_object *pred=succ->pred;
		
		head->pred=pred;
		tail->succ=succ;
		pred->succ=head;
		succ->pred=tail;
		
		gc_free_bytes+=reclaimed_bytes;
	}

	//move marked to unmarked.
	if( GC_LIST_IS_EMPTY( gc_marked_list ) ){
		GC_CLEAR_LIST( gc_unmarked_list );
	}else{
		gc_unmarked_list.succ=gc_marked_list.succ;
		gc_unmarked_list.pred=gc_marked_list.pred;
		gc_unmarked_list.succ->pred=gc_unmarked_list.pred->succ=&gc_unmarked_list;
		GC_CLEAR_LIST( gc_marked_list );
	}
	
	//adjust sizes
	gc_alloced_bytes=gc_marked_bytes;
	gc_marked_bytes=0;
	gc_markbit^=1;
}

void gc_collect_all(){

//	puts( "Mark locals" );
	gc_mark_locals();

//	puts( "Marked queued" );
	gc_mark_queued( 0x7fffffff );

//	puts( "Sweep" );
	gc_sweep();

//	puts( "Mark roots" );
	gc_mark_roots();

#if DEBUG_GC
	gc_validate_list( gc_marked_list,"Validating gc_marked_list"  );
	gc_validate_list( gc_unmarked_list,"Validating gc_unmarked_list"  );
	gc_validate_list( gc_free_list,"Validating gc_free_list" );
#endif

}

void gc_collect(){
	
#if CFG_CPP_GC_MODE==1

#if DEBUG_GC
	int ms=gc_micros();
#endif

	if( gc_new_bytes+gc_ext_new_bytes>(CFG_CPP_GC_TRIGGER) ){
		atomic_sub( &gc_ext_new_bytes,gc_ext_new_bytes );
		gc_collect_all();
		gc_new_bytes=0;
	}else{
		gc_mark_queued( (long long)(gc_new_bytes)*(gc_alloced_bytes-gc_new_bytes)/(CFG_CPP_GC_TRIGGER)+gc_new_bytes );
	}

#if DEBUG_GC
	ms=gc_micros()-ms;
//	if( ms>=100 ) {printf( "gc time:%i\n",ms );fflush( stdout );}
	if( ms>10 ) {printf( "gc time:%i\n",ms );fflush( stdout );}
#endif

#endif
}

// ***** Array *****

template<class T> T *t_memcpy( T *dst,const T *src,int n ){
	memcpy( dst,src,n*sizeof(T) );
	return dst+n;
}

template<class T> T *t_memset( T *dst,int val,int n ){
	memset( dst,val,n*sizeof(T) );
	return dst+n;
}

template<class T> int t_memcmp( const T *x,const T *y,int n ){
	return memcmp( x,y,n*sizeof(T) );
}

template<class T> int t_strlen( const T *p ){
	const T *q=p++;
	while( *q++ ){}
	return q-p;
}

template<class T> T *t_create( int n,T *p ){
	t_memset( p,0,n );
	return p+n;
}

template<class T> T *t_create( int n,T *p,const T *q ){
	t_memcpy( p,q,n );
	return p+n;
}

template<class T> void t_destroy( int n,T *p ){
}

template<class T> void gc_mark_elements( int n,T *p ){
}

template<class T> void gc_mark_elements( int n,T **p ){
	for( int i=0;i<n;++i ) gc_mark( p[i] );
}

template<class T> class Array{
public:
	Array():rep( &nullRep ){
	}

	//Uses default...
//	Array( const Array<T> &t )...
	
	Array( int length ):rep( Rep::alloc( length ) ){
		t_create( rep->length,rep->data );
	}
	
	Array( const T *p,int length ):rep( Rep::alloc(length) ){
		t_create( rep->length,rep->data,p );
	}
	
	~Array(){
	}

	//Uses default...
//	Array &operator=( const Array &t )...
	
	int Length()const{ 
		return rep->length; 
	}
	
	T &At( int index ){
		if( index<0 || index>=rep->length ) dbg_error( "Array index out of range" );
		return rep->data[index]; 
	}
	
	const T &At( int index )const{
		if( index<0 || index>=rep->length ) dbg_error( "Array index out of range" );
		return rep->data[index]; 
	}
	
	T &operator[]( int index ){
		return rep->data[index]; 
	}

	const T &operator[]( int index )const{
		return rep->data[index]; 
	}
	
	Array Slice( int from,int term )const{
		int len=rep->length;
		if( from<0 ){ 
			from+=len;
			if( from<0 ) from=0;
		}else if( from>len ){
			from=len;
		}
		if( term<0 ){
			term+=len;
		}else if( term>len ){
			term=len;
		}
		if( term<=from ) return Array();
		return Array( rep->data+from,term-from );
	}

	Array Slice( int from )const{
		return Slice( from,rep->length );
	}
	
	Array Resize( int newlen )const{
		if( newlen<=0 ) return Array();
		int n=rep->length;
		if( newlen<n ) n=newlen;
		Rep *p=Rep::alloc( newlen );
		T *q=p->data;
		q=t_create( n,q,rep->data );
		q=t_create( (newlen-n),q );
		return Array( p );
	}
	
private:
	struct Rep : public gc_object{
		int length;
		T data[0];
		
		Rep():length(0){
			flags=3;
		}
		
		Rep( int length ):length(length){
		}
		
		~Rep(){
			t_destroy( length,data );
		}
		
		void mark(){
			gc_mark_elements( length,data );
		}
		
		static Rep *alloc( int length ){
			if( !length ) return &nullRep;
			void *p=gc_object_alloc( sizeof(Rep)+length*sizeof(T) );
			return ::new(p) Rep( length );
		}
		
	};
	Rep *rep;
	
	static Rep nullRep;
	
	template<class C> friend void gc_mark( Array<C> t );
	template<class C> friend void gc_mark_q( Array<C> t );
	template<class C> friend Array<C> gc_retain( Array<C> t );
	template<class C> friend void gc_assign( Array<C> &lhs,Array<C> rhs );
	template<class C> friend void gc_mark_elements( int n,Array<C> *p );
	
	Array( Rep *rep ):rep(rep){
	}
};

template<class T> typename Array<T>::Rep Array<T>::nullRep;

template<class T> Array<T> *t_create( int n,Array<T> *p ){
	for( int i=0;i<n;++i ) *p++=Array<T>();
	return p;
}

template<class T> Array<T> *t_create( int n,Array<T> *p,const Array<T> *q ){
	for( int i=0;i<n;++i ) *p++=*q++;
	return p;
}

template<class T> void gc_mark( Array<T> t ){
	gc_mark( t.rep );
}

template<class T> void gc_mark_q( Array<T> t ){
	gc_mark_q( t.rep );
}

template<class T> Array<T> gc_retain( Array<T> t ){
#if CFG_CPP_GC_MODE==2
	gc_retain( t.rep );
#endif
	return t;
}

template<class T> void gc_assign( Array<T> &lhs,Array<T> rhs ){
	gc_mark( rhs.rep );
	lhs=rhs;
}

template<class T> void gc_mark_elements( int n,Array<T> *p ){
	for( int i=0;i<n;++i ) gc_mark( p[i].rep );
}
		
// ***** String *****

static const char *_str_load_err;

class String{
public:
	String():rep( &nullRep ){
	}
	
	String( const String &t ):rep( t.rep ){
		rep->retain();
	}

	String( int n ){
		char buf[256];
		sprintf_s( buf,"%i",n );
		rep=Rep::alloc( t_strlen(buf) );
		for( int i=0;i<rep->length;++i ) rep->data[i]=buf[i];
	}
	
	String( Float n ){
		char buf[256];
		
		//would rather use snprintf, but it's doing weird things in MingW.
		//
		sprintf_s( buf,"%.17lg",n );
		//
		char *p;
		for( p=buf;*p;++p ){
			if( *p=='.' || *p=='e' ) break;
		}
		if( !*p ){
			*p++='.';
			*p++='0';
			*p=0;
		}

		rep=Rep::alloc( t_strlen(buf) );
		for( int i=0;i<rep->length;++i ) rep->data[i]=buf[i];
	}

	String( Char ch,int length ):rep( Rep::alloc(length) ){
		for( int i=0;i<length;++i ) rep->data[i]=ch;
	}

	String( const Char *p ):rep( Rep::alloc(t_strlen(p)) ){
		t_memcpy( rep->data,p,rep->length );
	}

	String( const Char *p,int length ):rep( Rep::alloc(length) ){
		t_memcpy( rep->data,p,rep->length );
	}
	
#if __OBJC__	
	String( NSString *nsstr ):rep( Rep::alloc([nsstr length]) ){
		unichar *buf=(unichar*)malloc( rep->length * sizeof(unichar) );
		[nsstr getCharacters:buf range:NSMakeRange(0,rep->length)];
		for( int i=0;i<rep->length;++i ) rep->data[i]=buf[i];
		free( buf );
	}
#endif

#if __cplusplus_winrt
	String( Platform::String ^str ):rep( Rep::alloc(str->Length()) ){
		for( int i=0;i<rep->length;++i ) rep->data[i]=str->Data()[i];
	}
#endif

	~String(){
		rep->release();
	}
	
	template<class C> String( const C *p ):rep( Rep::alloc(t_strlen(p)) ){
		for( int i=0;i<rep->length;++i ) rep->data[i]=p[i];
	}
	
	template<class C> String( const C *p,int length ):rep( Rep::alloc(length) ){
		for( int i=0;i<rep->length;++i ) rep->data[i]=p[i];
	}
	
	String Copy()const{
		Rep *crep=Rep::alloc( rep->length );
		t_memcpy( crep->data,rep->data,rep->length );
		return String( crep );
	}
	
	int Length()const{
		return rep->length;
	}
	
	const Char *Data()const{
		return rep->data;
	}
	
	Char At( int index )const{
		if( index<0 || index>=rep->length ) dbg_error( "Character index out of range" );
		return rep->data[index]; 
	}
	
	Char operator[]( int index )const{
		return rep->data[index];
	}
	
	String &operator=( const String &t ){
		t.rep->retain();
		rep->release();
		rep=t.rep;
		return *this;
	}
	
	String &operator+=( const String &t ){
		return operator=( *this+t );
	}
	
	int Compare( const String &t )const{
		int n=rep->length<t.rep->length ? rep->length : t.rep->length;
		for( int i=0;i<n;++i ){
			if( int q=(int)(rep->data[i])-(int)(t.rep->data[i]) ) return q;
		}
		return rep->length-t.rep->length;
	}
	
	bool operator==( const String &t )const{
		return rep->length==t.rep->length && t_memcmp( rep->data,t.rep->data,rep->length )==0;
	}
	
	bool operator!=( const String &t )const{
		return rep->length!=t.rep->length || t_memcmp( rep->data,t.rep->data,rep->length )!=0;
	}
	
	bool operator<( const String &t )const{
		return Compare( t )<0;
	}
	
	bool operator<=( const String &t )const{
		return Compare( t )<=0;
	}
	
	bool operator>( const String &t )const{
		return Compare( t )>0;
	}
	
	bool operator>=( const String &t )const{
		return Compare( t )>=0;
	}
	
	String operator+( const String &t )const{
		if( !rep->length ) return t;
		if( !t.rep->length ) return *this;
		Rep *p=Rep::alloc( rep->length+t.rep->length );
		Char *q=p->data;
		q=t_memcpy( q,rep->data,rep->length );
		q=t_memcpy( q,t.rep->data,t.rep->length );
		return String( p );
	}
	
	int Find( String find,int start=0 )const{
		if( start<0 ) start=0;
		while( start+find.rep->length<=rep->length ){
			if( !t_memcmp( rep->data+start,find.rep->data,find.rep->length ) ) return start;
			++start;
		}
		return -1;
	}
	
	int FindLast( String find )const{
		int start=rep->length-find.rep->length;
		while( start>=0 ){
			if( !t_memcmp( rep->data+start,find.rep->data,find.rep->length ) ) return start;
			--start;
		}
		return -1;
	}
	
	int FindLast( String find,int start )const{
		if( start>rep->length-find.rep->length ) start=rep->length-find.rep->length;
		while( start>=0 ){
			if( !t_memcmp( rep->data+start,find.rep->data,find.rep->length ) ) return start;
			--start;
		}
		return -1;
	}
	
	String Trim()const{
		int i=0,i2=rep->length;
		while( i<i2 && rep->data[i]<=32 ) ++i;
		while( i2>i && rep->data[i2-1]<=32 ) --i2;
		if( i==0 && i2==rep->length ) return *this;
		return String( rep->data+i,i2-i );
	}

	Array<String> Split( String sep )const{
	
		if( !sep.rep->length ){
			Array<String> bits( rep->length );
			for( int i=0;i<rep->length;++i ){
				bits[i]=String( (Char)(*this)[i],1 );
			}
			return bits;
		}
		
		int i=0,i2,n=1;
		while( (i2=Find( sep,i ))!=-1 ){
			++n;
			i=i2+sep.rep->length;
		}
		Array<String> bits( n );
		if( n==1 ){
			bits[0]=*this;
			return bits;
		}
		i=0;n=0;
		while( (i2=Find( sep,i ))!=-1 ){
			bits[n++]=Slice( i,i2 );
			i=i2+sep.rep->length;
		}
		bits[n]=Slice( i );
		return bits;
	}

	String Join( Array<String> bits )const{
		if( bits.Length()==0 ) return String();
		if( bits.Length()==1 ) return bits[0];
		int newlen=rep->length * (bits.Length()-1);
		for( int i=0;i<bits.Length();++i ){
			newlen+=bits[i].rep->length;
		}
		Rep *p=Rep::alloc( newlen );
		Char *q=p->data;
		q=t_memcpy( q,bits[0].rep->data,bits[0].rep->length );
		for( int i=1;i<bits.Length();++i ){
			q=t_memcpy( q,rep->data,rep->length );
			q=t_memcpy( q,bits[i].rep->data,bits[i].rep->length );
		}
		return String( p );
	}

	String Replace( String find,String repl )const{
		int i=0,i2,newlen=0;
		while( (i2=Find( find,i ))!=-1 ){
			newlen+=(i2-i)+repl.rep->length;
			i=i2+find.rep->length;
		}
		if( !i ) return *this;
		newlen+=rep->length-i;
		Rep *p=Rep::alloc( newlen );
		Char *q=p->data;
		i=0;
		while( (i2=Find( find,i ))!=-1 ){
			q=t_memcpy( q,rep->data+i,i2-i );
			q=t_memcpy( q,repl.rep->data,repl.rep->length );
			i=i2+find.rep->length;
		}
		q=t_memcpy( q,rep->data+i,rep->length-i );
		return String( p );
	}

	String ToLower()const{
		for( int i=0;i<rep->length;++i ){
			Char t=towlower( rep->data[i] );
			if( t==rep->data[i] ) continue;
			Rep *p=Rep::alloc( rep->length );
			Char *q=p->data;
			t_memcpy( q,rep->data,i );
			for( q[i++]=t;i<rep->length;++i ){
				q[i]=towlower( rep->data[i] );
			}
			return String( p );
		}
		return *this;
	}

	String ToUpper()const{
		for( int i=0;i<rep->length;++i ){
			Char t=towupper( rep->data[i] );
			if( t==rep->data[i] ) continue;
			Rep *p=Rep::alloc( rep->length );
			Char *q=p->data;
			t_memcpy( q,rep->data,i );
			for( q[i++]=t;i<rep->length;++i ){
				q[i]=towupper( rep->data[i] );
			}
			return String( p );
		}
		return *this;
	}
	
	bool Contains( String sub )const{
		return Find( sub )!=-1;
	}

	bool StartsWith( String sub )const{
		return sub.rep->length<=rep->length && !t_memcmp( rep->data,sub.rep->data,sub.rep->length );
	}

	bool EndsWith( String sub )const{
		return sub.rep->length<=rep->length && !t_memcmp( rep->data+rep->length-sub.rep->length,sub.rep->data,sub.rep->length );
	}
	
	String Slice( int from,int term )const{
		int len=rep->length;
		if( from<0 ){
			from+=len;
			if( from<0 ) from=0;
		}else if( from>len ){
			from=len;
		}
		if( term<0 ){
			term+=len;
		}else if( term>len ){
			term=len;
		}
		if( term<from ) return String();
		if( from==0 && term==len ) return *this;
		return String( rep->data+from,term-from );
	}

	String Slice( int from )const{
		return Slice( from,rep->length );
	}
	
	Array<int> ToChars()const{
		Array<int> chars( rep->length );
		for( int i=0;i<rep->length;++i ) chars[i]=rep->data[i];
		return chars;
	}
	
	int ToInt()const{
		char buf[64];
		return atoi( ToCString<char>( buf,sizeof(buf) ) );
	}
	
	Float ToFloat()const{
		char buf[256];
		return atof( ToCString<char>( buf,sizeof(buf) ) );
	}

	template<class C> class CString{
		struct Rep{
			int refs;
			C data[1];
		};
		Rep *_rep;
		static Rep _nul;
	public:
		template<class T> CString( const T *data,int length ){
			_rep=(Rep*)malloc( length*sizeof(C)+sizeof(Rep) );
			_rep->refs=1;
			_rep->data[length]=0;
			for( int i=0;i<length;++i ){
				_rep->data[i]=(C)data[i];
			}
		}
		CString():_rep( new Rep ){
			_rep->refs=1;
		}
		CString( const CString &c ):_rep(c._rep){
			++_rep->refs;
		}
		~CString(){
			if( !--_rep->refs ) free( _rep );
		}
		CString &operator=( const CString &c ){
			++c._rep->refs;
			if( !--_rep->refs ) free( _rep );
			_rep=c._rep;
			return *this;
		}
		operator const C*()const{ 
			return _rep->data;
		}
	};
	
	template<class C> CString<C> ToCString()const{
		return CString<C>( rep->data,rep->length );
	}

	template<class C> C *ToCString( C *p,int length )const{
		if( --length>rep->length ) length=rep->length;
		for( int i=0;i<length;++i ) p[i]=rep->data[i];
		p[length]=0;
		return p;
	}
	
#if __OBJC__	
	NSString *ToNSString()const{
		return [NSString stringWithCharacters:ToCString<unichar>() length:rep->length];
	}
#endif

#if __cplusplus_winrt
	Platform::String ^ToWinRTString()const{
		return ref new Platform::String( rep->data,rep->length );
	}
#endif
	CString<char> ToUtf8()const{
		std::vector<unsigned char> buf;
		Save( buf );
		return CString<char>( &buf[0],buf.size() );
	}

	bool Save( FILE *fp )const{
		std::vector<unsigned char> buf;
		Save( buf );
		return buf.size() ? fwrite( &buf[0],1,buf.size(),fp )==buf.size() : true;
	}
	
	void Save( std::vector<unsigned char> &buf )const{
	
		Char *p=rep->data;
		Char *e=p+rep->length;
		
		while( p<e ){
			Char c=*p++;
			if( c<0x80 ){
				buf.push_back( c );
			}else if( c<0x800 ){
				buf.push_back( 0xc0 | (c>>6) );
				buf.push_back( 0x80 | (c & 0x3f) );
			}else{
				buf.push_back( 0xe0 | (c>>12) );
				buf.push_back( 0x80 | ((c>>6) & 0x3f) );
				buf.push_back( 0x80 | (c & 0x3f) );
			}
		}
	}
	
	static String FromChars( Array<int> chars ){
		int n=chars.Length();
		Rep *p=Rep::alloc( n );
		for( int i=0;i<n;++i ){
			p->data[i]=chars[i];
		}
		return String( p );
	}

	static String Load( FILE *fp ){
		unsigned char tmp[4096];
		std::vector<unsigned char> buf;
		for(;;){
			int n=fread( tmp,1,4096,fp );
			if( n>0 ) buf.insert( buf.end(),tmp,tmp+n );
			if( n!=4096 ) break;
		}
		return buf.size() ? String::Load( &buf[0],buf.size() ) : String();
	}
	
	static String Load( unsigned char *p,int n ){
	
		_str_load_err=0;
		
		unsigned char *e=p+n;
		std::vector<Char> chars;
		
		int t0=n>0 ? p[0] : -1;
		int t1=n>1 ? p[1] : -1;

		if( t0==0xfe && t1==0xff ){
			p+=2;
			while( p<e-1 ){
				int c=*p++;
				chars.push_back( (c<<8)|*p++ );
			}
		}else if( t0==0xff && t1==0xfe ){
			p+=2;
			while( p<e-1 ){
				int c=*p++;
				chars.push_back( (*p++<<8)|c );
			}
		}else{
			int t2=n>2 ? p[2] : -1;
			if( t0==0xef && t1==0xbb && t2==0xbf ) p+=3;
			unsigned char *q=p;
			bool fail=false;
			while( p<e ){
				unsigned int c=*p++;
				if( c & 0x80 ){
					if( (c & 0xe0)==0xc0 ){
						if( p>=e || (p[0] & 0xc0)!=0x80 ){
							fail=true;
							break;
						}
						c=((c & 0x1f)<<6) | (p[0] & 0x3f);
						p+=1;
					}else if( (c & 0xf0)==0xe0 ){
						if( p+1>=e || (p[0] & 0xc0)!=0x80 || (p[1] & 0xc0)!=0x80 ){
							fail=true;
							break;
						}
						c=((c & 0x0f)<<12) | ((p[0] & 0x3f)<<6) | (p[1] & 0x3f);
						p+=2;
					}else{
						fail=true;
						break;
					}
				}
				chars.push_back( c );
			}
			if( fail ){
				_str_load_err="Invalid UTF-8";
				return String( q,n );
			}
		}
		return chars.size() ? String( &chars[0],chars.size() ) : String();
	}

private:
	
	struct Rep{
		int refs;
		int length;
		Char data[0];
		
		Rep():refs(1),length(0){
		}
		
		Rep( int length ):refs(1),length(length){
		}
		
		void retain(){
			++refs;
		}
		
		void release(){
			if( --refs || this==&nullRep ) return;
			gc_new_bytes-=sizeof(Rep)+length*sizeof(Char);
			free( this );
		}

		static Rep *alloc( int length ){
			if( !length ) return &nullRep;
			void *p=malloc( sizeof(Rep)+length*sizeof(Char) );
			gc_new_bytes+=sizeof(Rep)+length*sizeof(Char);
			return new(p) Rep( length );
		}
	};
	Rep *rep;
	
	static Rep nullRep;
	
	String( Rep *rep ):rep(rep){
	}
};

String::Rep String::nullRep;

String *t_create( int n,String *p ){
	for( int i=0;i<n;++i ) new( &p[i] ) String();
	return p+n;
}

String *t_create( int n,String *p,const String *q ){
	for( int i=0;i<n;++i ) new( &p[i] ) String( q[i] );
	return p+n;
}

void t_destroy( int n,String *p ){
	for( int i=0;i<n;++i ) p[i].~String();
}

// ***** Object *****

String dbg_stacktrace();

class Object : public gc_object{
public:
	virtual bool Equals( Object *obj ){
		return this==obj;
	}
	
	virtual int Compare( Object *obj ){
		return (char*)this-(char*)obj;
	}
	
	virtual String debug(){
		return "+Object\n";
	}
};

class ThrowableObject : public Object{
#ifndef NDEBUG
public:
	String stackTrace;
	ThrowableObject():stackTrace( dbg_stacktrace() ){}
#endif
};

struct gc_interface{
	virtual ~gc_interface(){}
};

//***** Debugger *****

//#define Error bbError
//#define Print bbPrint

int bbPrint( String t );

#define dbg_stream stderr

#if _MSC_VER
#define dbg_typeof decltype
#else
#define dbg_typeof __typeof__
#endif 

struct dbg_func;
struct dbg_var_type;

static int dbg_suspend;
static int dbg_stepmode;

const char *dbg_info;
String dbg_exstack;

static void *dbg_var_buf[65536*3];
static void **dbg_var_ptr=dbg_var_buf;

static dbg_func *dbg_func_buf[1024];
static dbg_func **dbg_func_ptr=dbg_func_buf;

String dbg_type( bool *p ){
	return "Bool";
}

String dbg_type( int *p ){
	return "Int";
}

String dbg_type( Float *p ){
	return "Float";
}

String dbg_type( String *p ){
	return "String";
}

template<class T> String dbg_type( T **p ){
	return "Object";
}

template<class T> String dbg_type( Array<T> *p ){
	return dbg_type( &(*p)[0] )+"[]";
}

String dbg_value( bool *p ){
	return *p ? "True" : "False";
}

String dbg_value( int *p ){
	return String( *p );
}

String dbg_value( Float *p ){
	return String( *p );
}

String dbg_value( String *p ){
	String t=*p;
	if( t.Length()>100 ) t=t.Slice( 0,100 )+"...";
	t=t.Replace( "\"","~q" );
	t=t.Replace( "\t","~t" );
	t=t.Replace( "\n","~n" );
	t=t.Replace( "\r","~r" );
	return String("\"")+t+"\"";
}

template<class T> String dbg_value( T **t ){
	Object *p=dynamic_cast<Object*>( *t );
	char buf[64];
	sprintf_s( buf,"%p",p );
	return String("@") + (buf[0]=='0' && buf[1]=='x' ? buf+2 : buf );
}

template<class T> String dbg_value( Array<T> *p ){
	String t="[";
	int n=(*p).Length();
	if( n>100 ) n=100;
	for( int i=0;i<n;++i ){
		if( i ) t+=",";
		t+=dbg_value( &(*p)[i] );
	}
	return t+"]";
}

String dbg_ptr_value( void *p ){
	char buf[64];
	sprintf_s( buf,"%p",p );
	return (buf[0]=='0' && buf[1]=='x' ? buf+2 : buf );
}

template<class T> String dbg_decl( const char *id,T *ptr ){
	return String( id )+":"+dbg_type(ptr)+"="+dbg_value(ptr)+"\n";
}

struct dbg_var_type{
	virtual String type( void *p )=0;
	virtual String value( void *p )=0;
};

template<class T> struct dbg_var_type_t : public dbg_var_type{

	String type( void *p ){
		return dbg_type( (T*)p );
	}
	
	String value( void *p ){
		return dbg_value( (T*)p );
	}
	
	static dbg_var_type_t<T> info;
};
template<class T> dbg_var_type_t<T> dbg_var_type_t<T>::info;

struct dbg_blk{
	void **var_ptr;
	
	dbg_blk():var_ptr(dbg_var_ptr){
		if( dbg_stepmode=='l' ) --dbg_suspend;
	}
	
	~dbg_blk(){
		if( dbg_stepmode=='l' ) ++dbg_suspend;
		dbg_var_ptr=var_ptr;
	}
};

struct dbg_func : public dbg_blk{
	const char *id;
	const char *info;

	dbg_func( const char *p ):id(p),info(dbg_info){
		*dbg_func_ptr++=this;
		if( dbg_stepmode=='s' ) --dbg_suspend;
	}
	
	~dbg_func(){
		if( dbg_stepmode=='s' ) ++dbg_suspend;
		--dbg_func_ptr;
		dbg_info=info;
	}
};

int dbg_print( String t ){
	static char *buf;
	static int len;
	int n=t.Length();
	if( n+100>len ){
		len=n+100;
		free( buf );
		buf=(char*)malloc( len );
	}
	buf[n]='\n';
	for( int i=0;i<n;++i ) buf[i]=t[i];
	fwrite( buf,n+1,1,dbg_stream );
	fflush( dbg_stream );
	return 0;
}

void dbg_callstack(){

	void **var_ptr=dbg_var_buf;
	dbg_func **func_ptr=dbg_func_buf;
	
	while( var_ptr!=dbg_var_ptr ){
		while( func_ptr!=dbg_func_ptr && var_ptr==(*func_ptr)->var_ptr ){
			const char *id=(*func_ptr++)->id;
			const char *info=func_ptr!=dbg_func_ptr ? (*func_ptr)->info : dbg_info;
			fprintf( dbg_stream,"+%s;%s\n",id,info );
		}
		void *vp=*var_ptr++;
		const char *nm=(const char*)*var_ptr++;
		dbg_var_type *ty=(dbg_var_type*)*var_ptr++;
		dbg_print( String(nm)+":"+ty->type(vp)+"="+ty->value(vp) );
	}
	while( func_ptr!=dbg_func_ptr ){
		const char *id=(*func_ptr++)->id;
		const char *info=func_ptr!=dbg_func_ptr ? (*func_ptr)->info : dbg_info;
		fprintf( dbg_stream,"+%s;%s\n",id,info );
	}
}

String dbg_stacktrace(){
	if( !dbg_info || !dbg_info[0] ) return "";
	String str=String( dbg_info )+"\n";
	dbg_func **func_ptr=dbg_func_ptr;
	if( func_ptr==dbg_func_buf ) return str;
	while( --func_ptr!=dbg_func_buf ){
		str+=String( (*func_ptr)->info )+"\n";
	}
	return str;
}

void dbg_throw( const char *err ){
	dbg_exstack=dbg_stacktrace();
	throw err;
}

void dbg_stop(){

#if TARGET_OS_IPHONE
	dbg_throw( "STOP" );
#endif

	fprintf( dbg_stream,"{{~~%s~~}}\n",dbg_info );
	dbg_callstack();
	dbg_print( "" );
	
	for(;;){

		char buf[256];
		char *e=fgets( buf,256,stdin );
		if( !e ) exit( -1 );
		
		e=strchr( buf,'\n' );
		if( !e ) exit( -1 );
		
		*e=0;
		
		Object *p;
		
		switch( buf[0] ){
		case '?':
			break;
		case 'r':	//run
			dbg_suspend=0;		
			dbg_stepmode=0;
			return;
		case 's':	//step
			dbg_suspend=1;
			dbg_stepmode='s';
			return;
		case 'e':	//enter func
			dbg_suspend=1;
			dbg_stepmode='e';
			return;
		case 'l':	//leave block
			dbg_suspend=0;
			dbg_stepmode='l';
			return;
		case '@':	//dump object
			p=0;
			sscanf_s( buf+1,"%p",&p );
			if( p ){
				dbg_print( p->debug() );
			}else{
				dbg_print( "" );
			}
			break;
		case 'q':	//quit!
			exit( 0 );
			break;			
		default:
			printf( "????? %s ?????",buf );fflush( stdout );
			exit( -1 );
		}
	}
}

void dbg_error( const char *err ){

#if TARGET_OS_IPHONE
	dbg_throw( err );
#endif

	for(;;){
		bbPrint( String("Cerberus Runtime Error : ")+err );
		bbPrint( dbg_stacktrace() );
		dbg_stop();
	}
}

#define DBG_INFO(X) dbg_info=(X);if( dbg_suspend>0 ) dbg_stop();

#define DBG_ENTER(P) dbg_func _dbg_func(P);

#define DBG_BLOCK() dbg_blk _dbg_blk;

#define DBG_GLOBAL( ID,NAME )	//TODO!

#define DBG_LOCAL( ID,NAME )\
*dbg_var_ptr++=&ID;\
*dbg_var_ptr++=(void*)NAME;\
*dbg_var_ptr++=&dbg_var_type_t<dbg_typeof(ID)>::info;

//**** main ****

int argc;
const char **argv;

Float D2R=0.017453292519943295f;
Float R2D=57.29577951308232f;

int bbPrint( String t ){

	static std::vector<unsigned char> buf;
	buf.clear();
	t.Save( buf );
	buf.push_back( '\n' );
	buf.push_back( 0 );
	
#if __cplusplus_winrt	//winrt?

#if CFG_WINRT_PRINT_ENABLED
	OutputDebugStringA( (const char*)&buf[0] );
#endif

#elif _WIN32			//windows?

	fputs( (const char*)&buf[0],stdout );
	fflush( stdout );

#elif __APPLE__			//macos/ios?

	fputs( (const char*)&buf[0],stdout );
	fflush( stdout );
	
#elif __linux			//linux?

#if CFG_ANDROID_NDK_PRINT_ENABLED
	LOGI( (const char*)&buf[0] );
#else
	fputs( (const char*)&buf[0],stdout );
	fflush( stdout );
#endif

#endif

	return 0;
}

class BBExitApp{
};

int bbError( String err ){
	if( !err.Length() ){
#if __cplusplus_winrt
		throw BBExitApp();
#else
		exit( 0 );
#endif
	}
	dbg_error( err.ToCString<char>() );
	return 0;
}

int bbDebugLog( String t ){
	bbPrint( t );
	return 0;
}

int bbDebugStop(){
	dbg_stop();
	return 0;
}

int bbInit();
int bbMain();

#if _MSC_VER

static void _cdecl seTranslator( unsigned int ex,EXCEPTION_POINTERS *p ){

	switch( ex ){
	case EXCEPTION_ACCESS_VIOLATION:dbg_error( "Memory access violation" );
	case EXCEPTION_ILLEGAL_INSTRUCTION:dbg_error( "Illegal instruction" );
	case EXCEPTION_INT_DIVIDE_BY_ZERO:dbg_error( "Integer divide by zero" );
	case EXCEPTION_STACK_OVERFLOW:dbg_error( "Stack overflow" );
	}
	dbg_error( "Unknown exception" );
}

#else

void sighandler( int sig  ){
	switch( sig ){
	case SIGSEGV:dbg_error( "Memory access violation" );
	case SIGILL:dbg_error( "Illegal instruction" );
	case SIGFPE:dbg_error( "Floating point exception" );
#if !_WIN32
	case SIGBUS:dbg_error( "Bus error" );
#endif	
	}
	dbg_error( "Unknown signal" );
}

#endif

//entry point call by target main()...
//
int bb_std_main( int argc,const char **argv ){

	::argc=argc;
	::argv=argv;
	
#if _MSC_VER

	_set_se_translator( seTranslator );

#else
	
	signal( SIGSEGV,sighandler );
	signal( SIGILL,sighandler );
	signal( SIGFPE,sighandler );
#if !_WIN32
	signal( SIGBUS,sighandler );
#endif

#endif

	if( !setlocale( LC_CTYPE,"en_US.UTF-8" ) ){
		setlocale( LC_CTYPE,"" );
	}

	gc_init1();

	bbInit();
	
	gc_init2();

	bbMain();

	return 0;
}


//***** game.h *****

struct BBGameEvent{
	enum{
		None=0,
		KeyDown=1,KeyUp=2,KeyChar=3,
		MouseDown=4,MouseUp=5,MouseMove=6,
		TouchDown=7,TouchUp=8,TouchMove=9,
		MotionAccel=10
	};
};

class BBGameDelegate : public Object{
public:
	virtual void StartGame(){}
	virtual void SuspendGame(){}
	virtual void ResumeGame(){}
	virtual void UpdateGame(){}
	virtual void RenderGame(){}
	virtual void KeyEvent( int event,int data ){}
	virtual void MouseEvent( int event,int data,Float x,Float y, Float z ){}
	virtual void TouchEvent( int event,int data,Float x,Float y ){}
	virtual void MotionEvent( int event,int data,Float x,Float y,Float z ){}
	virtual void DiscardGraphics(){}
};

struct BBDisplayMode : public Object{
	int width;
	int height;
	int depth;
	int hertz;
	int flags;
	BBDisplayMode( int width=0,int height=0,int depth=0,int hertz=0,int flags=0 ):width(width),height(height),depth(depth),hertz(hertz),flags(flags){}
};

class BBGame{
public:
	BBGame();
	virtual ~BBGame(){}
	
	// ***** Extern *****
	static BBGame *Game(){ return _game; }
	
	virtual void SetDelegate( BBGameDelegate *delegate );
	virtual BBGameDelegate *Delegate(){ return _delegate; }
	
	virtual void SetKeyboardEnabled( bool enabled );
	virtual bool KeyboardEnabled();
	
	virtual void SetUpdateRate( int updateRate );
	virtual int UpdateRate();
	
	virtual bool Started(){ return _started; }
	virtual bool Suspended(){ return _suspended; }
	
	virtual int Millisecs();
	virtual void GetDate( Array<int> date );
	virtual int SaveState( String state );
	virtual String LoadState();
	virtual String LoadString( String path );
	virtual int CountJoysticks( bool update );
	virtual bool PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons );
	virtual void OpenUrl( String url );
	virtual void SetMouseVisible( bool visible );
	
	virtual int GetDeviceWidth(){ return 0; }
	virtual int GetDeviceHeight(){ return 0; }
	virtual void SetDeviceWindow( int width,int height,int flags ){}
	virtual Array<BBDisplayMode*> GetDisplayModes(){ return Array<BBDisplayMode*>(); }
	virtual BBDisplayMode *GetDesktopMode(){ return 0; }
	virtual void SetSwapInterval( int interval ){}

	// ***** Native *****
	virtual String PathToFilePath( String path );
	virtual FILE *OpenFile( String path,String mode );
	virtual unsigned char *LoadData( String path,int *plength );
	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *depth ){ return 0; }
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz ){ return 0; }
	
	//***** Internal *****
	virtual void Die( ThrowableObject *ex );
	virtual void gc_collect();
	virtual void StartGame();
	virtual void SuspendGame();
	virtual void ResumeGame();
	virtual void UpdateGame();
	virtual void RenderGame();
	virtual void KeyEvent( int ev,int data );
	virtual void MouseEvent( int ev,int data,float x,float y, float z );
	virtual void TouchEvent( int ev,int data,float x,float y );
	virtual void MotionEvent( int ev,int data,float x,float y,float z );
	virtual void DiscardGraphics();
	
protected:

	static BBGame *_game;

	BBGameDelegate *_delegate;
	bool _keyboardEnabled;
	int _updateRate;
	bool _started;
	bool _suspended;
};

//***** game.cpp *****

BBGame *BBGame::_game;

BBGame::BBGame():
_delegate( 0 ),
_keyboardEnabled( false ),
_updateRate( 0 ),
_started( false ),
_suspended( false ){
	_game=this;
}

void BBGame::SetDelegate( BBGameDelegate *delegate ){
	_delegate=delegate;
}

void BBGame::SetKeyboardEnabled( bool enabled ){
	_keyboardEnabled=enabled;
}

bool BBGame::KeyboardEnabled(){
	return _keyboardEnabled;
}

void BBGame::SetUpdateRate( int updateRate ){
	_updateRate=updateRate;
}

int BBGame::UpdateRate(){
	return _updateRate;
}

int BBGame::Millisecs(){
	return 0;
}

void BBGame::GetDate( Array<int> date ){
	int n=date.Length();
	if( n>0 ){
		time_t t=time( 0 );
		
#if _MSC_VER
		struct tm tii;
		struct tm *ti=&tii;
		localtime_s( ti,&t );
#else
		struct tm *ti=localtime( &t );
#endif

		date[0]=ti->tm_year+1900;
		if( n>1 ){ 
			date[1]=ti->tm_mon+1;
			if( n>2 ){
				date[2]=ti->tm_mday;
				if( n>3 ){
					date[3]=ti->tm_hour;
					if( n>4 ){
						date[4]=ti->tm_min;
						if( n>5 ){
							date[5]=ti->tm_sec;
							if( n>6 ){
								date[6]=0;
							}
						}
					}
				}
			}
		}
	}
}

int BBGame::SaveState( String state ){
	if( FILE *f=OpenFile( "./.cerberusstate","wb" ) ){
		bool ok=state.Save( f );
		fclose( f );
		return ok ? 0 : -2;
	}
	return -1;
}

String BBGame::LoadState(){
	if( FILE *f=OpenFile( "./.cerberusstate","rb" ) ){
		String str=String::Load( f );
		fclose( f );
		return str;
	}
	return "";
}

String BBGame::LoadString( String path ){
	if( FILE *fp=OpenFile( path,"rb" ) ){
		String str=String::Load( fp );
		fclose( fp );
		return str;
	}
	return "";
}

int BBGame::CountJoysticks( bool update ){
	return 0;
}

bool BBGame::PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons ){
	return false;
}

void BBGame::OpenUrl( String url ){
}

void BBGame::SetMouseVisible( bool visible ){
}

//***** C++ Game *****

String BBGame::PathToFilePath( String path ){
	return path;
}

FILE *BBGame::OpenFile( String path,String mode ){
	path=PathToFilePath( path );
	if( path=="" ) return 0;
	
#if __cplusplus_winrt
	path=path.Replace( "/","\\" );
	FILE *f;
	if( _wfopen_s( &f,path.ToCString<wchar_t>(),mode.ToCString<wchar_t>() ) ) return 0;
	return f;
#elif _WIN32
	return _wfopen( path.ToCString<wchar_t>(),mode.ToCString<wchar_t>() );
#else
	return fopen( path.ToCString<char>(),mode.ToCString<char>() );
#endif
}

unsigned char *BBGame::LoadData( String path,int *plength ){

	FILE *f=OpenFile( path,"rb" );
	if( !f ) return 0;

	const int BUF_SZ=4096;
	std::vector<void*> tmps;
	int length=0;
	
	for(;;){
		void *p=malloc( BUF_SZ );
		int n=fread( p,1,BUF_SZ,f );
		tmps.push_back( p );
		length+=n;
		if( n!=BUF_SZ ) break;
	}
	fclose( f );
	
	unsigned char *data=(unsigned char*)malloc( length );
	unsigned char *p=data;
	
	int sz=length;
	for( int i=0;i<tmps.size();++i ){
		int n=sz>BUF_SZ ? BUF_SZ : sz;
		memcpy( p,tmps[i],n );
		free( tmps[i] );
		sz-=n;
		p+=n;
	}
	
	*plength=length;
	
	gc_ext_malloced( length );
	
	return data;
}

//***** INTERNAL *****

void BBGame::Die( ThrowableObject *ex ){
	bbPrint( "Cerberus Runtime Error : Uncaught Cerberus Exception" );
#ifndef NDEBUG
	bbPrint( ex->stackTrace );
#endif
	exit( -1 );
}

void BBGame::gc_collect(){
	gc_mark( _delegate );
	::gc_collect();
}

void BBGame::StartGame(){

	if( _started ) return;
	_started=true;
	
	try{
		_delegate->StartGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::SuspendGame(){

	if( !_started || _suspended ) return;
	_suspended=true;
	
	try{
		_delegate->SuspendGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::ResumeGame(){

	if( !_started || !_suspended ) return;
	_suspended=false;
	
	try{
		_delegate->ResumeGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::UpdateGame(){

	if( !_started || _suspended ) return;
	
	try{
		_delegate->UpdateGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::RenderGame(){

	if( !_started ) return;
	
	try{
		_delegate->RenderGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::KeyEvent( int ev,int data ){

	if( !_started ) return;
	
	try{
		_delegate->KeyEvent( ev,data );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::MouseEvent( int ev,int data,float x,float y, float z ){

	if( !_started ) return;
	
	try{
		_delegate->MouseEvent( ev,data,x,y,z );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::TouchEvent( int ev,int data,float x,float y ){

	if( !_started ) return;
	
	try{
		_delegate->TouchEvent( ev,data,x,y );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::MotionEvent( int ev,int data,float x,float y,float z ){

	if( !_started ) return;
	
	try{
		_delegate->MotionEvent( ev,data,x,y,z );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::DiscardGraphics(){

	if( !_started ) return;
	
	try{
		_delegate->DiscardGraphics();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

// Stdcpp trans.system runtime.
//
// Placed into the public domain 24/02/2011.
// No warranty implied; use as your own risk.

#if _WIN32

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

typedef WCHAR OS_CHAR;
typedef struct _stat stat_t;

#define mkdir( X,Y ) _wmkdir( X )
#define rmdir _wrmdir
#define remove _wremove
#define rename _wrename
#define stat _wstat
#define _fopen _wfopen
#define putenv _wputenv
#define getenv _wgetenv
#define system _wsystem
#define chdir _wchdir
#define getcwd _wgetcwd
#define realpath(X,Y) _wfullpath( Y,X,PATH_MAX )	//Note: first args SWAPPED to be posix-like!
#define opendir _wopendir
#define readdir _wreaddir
#define closedir _wclosedir
#define DIR _WDIR
#define dirent _wdirent

#elif __APPLE__

typedef char OS_CHAR;
typedef struct stat stat_t;

#define _fopen fopen

#elif __linux

/*
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
*/

typedef char OS_CHAR;
typedef struct stat stat_t;

#define _fopen fopen

#endif

static String _appPath;
static Array<String> _appArgs;

static String::CString<char> C_STR( const String &t ){
	return t.ToCString<char>();
}

static String::CString<OS_CHAR> OS_STR( const String &t ){
	return t.ToCString<OS_CHAR>();
}

String HostOS(){
#if _WIN32
	return "winnt";
#elif __APPLE__
	return "macos";
#elif __linux
	return "linux";
#else
	return "";
#endif
}

String RealPath( String path ){
	std::vector<OS_CHAR> buf( PATH_MAX+1 );
	if( realpath( OS_STR( path ),&buf[0] ) ){}
	buf[buf.size()-1]=0;
	for( int i=0;i<PATH_MAX && buf[i];++i ){
		if( buf[i]=='\\' ) buf[i]='/';
		
	}
	return String( &buf[0] );
}

String AppPath(){

	if( _appPath.Length() ) return _appPath;
	
#if _WIN32

	OS_CHAR buf[PATH_MAX+1];
	GetModuleFileNameW( GetModuleHandleW(0),buf,PATH_MAX );
	buf[PATH_MAX]=0;
	_appPath=String( buf );
	
#elif __APPLE__

	char buf[PATH_MAX];
	uint32_t size=sizeof( buf );
	_NSGetExecutablePath( buf,&size );
	buf[PATH_MAX-1]=0;
	_appPath=String( buf );
	
#elif __linux

	char lnk[PATH_MAX],buf[PATH_MAX];
	pid_t pid=getpid();
	sprintf( lnk,"/proc/%i/exe",pid );
	int i=readlink( lnk,buf,PATH_MAX );
	if( i>0 && i<PATH_MAX ){
		buf[i]=0;
		_appPath=String( buf );
	}

#endif

	_appPath=RealPath( _appPath );
	return _appPath;
}

Array<String> AppArgs(){
	if( _appArgs.Length() ) return _appArgs;
	_appArgs=Array<String>( argc );
	for( int i=0;i<argc;++i ){
		_appArgs[i]=String( argv[i] );
	}
	return _appArgs;
}
	
int FileType( String path ){
	stat_t st;
	if( stat( OS_STR(path),&st ) ) return 0;
	switch( st.st_mode & S_IFMT ){
	case S_IFREG : return 1;
	case S_IFDIR : return 2;
	}
	return 0;
}

int FileSize( String path ){
	stat_t st;
	if( stat( OS_STR(path),&st ) ) return -1;
	return st.st_size;
}

int FileTime( String path ){
	stat_t st;
	if( stat( OS_STR(path),&st ) ) return -1;
	return st.st_mtime;
}

String LoadString( String path ){
	if( FILE *fp=_fopen( OS_STR(path),OS_STR("rb") ) ){
		String str=String::Load( fp );
		if( _str_load_err ){
			bbPrint( String( _str_load_err )+" in file: "+path );
		}
		fclose( fp );
		return str;
	}
	return "";
}
	
int SaveString( String str,String path ){
	if( FILE *fp=_fopen( OS_STR(path),OS_STR("wb") ) ){
		bool ok=str.Save( fp );
		fclose( fp );
		return ok ? 0 : -2;
	}else{
//		printf( "FOPEN 'wb' for SaveString '%s' failed\n",C_STR( path ) );
		fflush( stdout );
	}
	return -1;
}

Array<String> LoadDir( String path ){
	std::vector<String> files;
	
#if _WIN32

	WIN32_FIND_DATAW filedata;
	HANDLE handle=FindFirstFileW( OS_STR(path+"/*"),&filedata );
	if( handle!=INVALID_HANDLE_VALUE ){
		do{
			String f=filedata.cFileName;
			if( f=="." || f==".." ) continue;
			files.push_back( f );
		}while( FindNextFileW( handle,&filedata ) );
		FindClose( handle );
	}else{
//		printf( "FindFirstFileW for LoadDir(%s) failed\n",C_STR(path) );
		fflush( stdout );
	}
	
#else

	if( DIR *dir=opendir( OS_STR(path) ) ){
		while( dirent *ent=readdir( dir ) ){
			String f=ent->d_name;
			if( f=="." || f==".." ) continue;
			files.push_back( f );
		}
		closedir( dir );
	}else{
//		printf( "opendir for LoadDir(%s) failed\n",C_STR(path) );
		fflush( stdout );
	}

#endif

	return files.size() ? Array<String>( &files[0],files.size() ) : Array<String>();
}
	
int CopyFile( String srcpath,String dstpath ){

#if _WIN32

	if( CopyFileW( OS_STR(srcpath),OS_STR(dstpath),FALSE ) ) return 1;
	return 0;
	
#elif __APPLE__

	// Would like to use COPY_ALL here, but it breaks trans on MacOS - produces weird 'pch out of date' error with copied projects.
	//
	// Ranlib strikes back!
	//
	if( copyfile( OS_STR(srcpath),OS_STR(dstpath),0,COPYFILE_DATA )>=0 ) return 1;
	return 0;
	
#else

	int err=-1;
	if( FILE *srcp=_fopen( OS_STR( srcpath ),OS_STR( "rb" ) ) ){
		err=-2;
		if( FILE *dstp=_fopen( OS_STR( dstpath ),OS_STR( "wb" ) ) ){
			err=0;
			char buf[1024];
			while( int n=fread( buf,1,1024,srcp ) ){
				if( fwrite( buf,1,n,dstp )!=n ){
					err=-3;
					break;
				}
			}
			fclose( dstp );
		}else{
//			printf( "FOPEN 'wb' for CopyFile(%s,%s) failed\n",C_STR(srcpath),C_STR(dstpath) );
			fflush( stdout );
		}
		fclose( srcp );
	}else{
//		printf( "FOPEN 'rb' for CopyFile(%s,%s) failed\n",C_STR(srcpath),C_STR(dstpath) );
		fflush( stdout );
	}
	return err==0;
	
#endif
}

int ChangeDir( String path ){
	return chdir( OS_STR(path) );
}

String CurrentDir(){
	std::vector<OS_CHAR> buf( PATH_MAX+1 );
	if( getcwd( &buf[0],buf.size() ) ){}
	buf[buf.size()-1]=0;
	return String( &buf[0] );
}

int CreateDir( String path ){
	mkdir( OS_STR( path ),0777 );
	return FileType(path)==2;
}

int DeleteDir( String path ){
	rmdir( OS_STR(path) );
	return FileType(path)==0;
}

int DeleteFile( String path ){
	remove( OS_STR(path) );
	return FileType(path)==0;
}

int SetEnv( String name,String value ){
#if _WIN32
	return putenv( OS_STR( name+"="+value ) );
#else
	if( value.Length() ) return setenv( OS_STR( name ),OS_STR( value ),1 );
	unsetenv( OS_STR( name ) );
	return 0;
#endif
}

String GetEnv( String name ){
	if( OS_CHAR *p=getenv( OS_STR(name) ) ) return String( p );
	return "";
}

int Execute( String cmd ){

#if _WIN32

	cmd=String("cmd /S /C \"")+cmd+"\"";

	PROCESS_INFORMATION pi={0};
	STARTUPINFOW si={sizeof(si)};

	if( !CreateProcessW( 0,(WCHAR*)(const OS_CHAR*)OS_STR(cmd),0,0,1,CREATE_DEFAULT_ERROR_MODE,0,0,&si,&pi ) ) return -1;

	WaitForSingleObject( pi.hProcess,INFINITE );

	int res=GetExitCodeProcess( pi.hProcess,(DWORD*)&res ) ? res : -1;

	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return res;

#else

	return system( OS_STR(cmd) );

#endif
}

int ExitApp( int retcode ){
	exit( retcode );
	return 0;
}

class c_ILinkResolver;
class c_IPrettifier;
class c_Makedocs;
class c_Stack;
class c_StringStack;
class c_List;
class c_StringList;
class c_Node;
class c_HeadNode;
class c_Enumerator;
class c_DocDecl;
class c_Stack2;
class c_DocDeclStack;
class c_Enumerator2;
class c_ApiDoccer;
class c_Toker;
class c_Parser;
class c_Set;
class c_StringSet;
class c_Map;
class c_StringMap;
class c_Node2;
class c_ThrowableString;
class c_Stack3;
class c_IntStack;
class c_BackwardsStack;
class c_BackwardsEnumerator;
class c_DocDoccer;
class c_Map2;
class c_StringMap2;
class c_Node3;
class c_Map3;
class c_StringMap3;
class c_Node4;
class c_Map4;
class c_StringMap4;
class c_Node5;
class c_NodeEnumerator;
class c_PageMaker;
class c_Stack4;
class c_Markdown;
class c_Stack5;
class c_StringObject;
class c_NodeEnumerator2;
class c_NodeEnumerator3;
class c_ILinkResolver : public virtual gc_interface{
	public:
	virtual String p_ResolveLink(String,String)=0;
};
class c_IPrettifier : public virtual gc_interface{
	public:
	virtual String p_PrettifyLine(String)=0;
	virtual String p_EndPrettyBlock()=0;
	virtual String p_BeginPrettyBlock()=0;
};
class c_Makedocs : public Object,public virtual c_ILinkResolver,public virtual c_IPrettifier{
	public:
	c_StringStack* m_ignoremods;
	String m_errinfofile;
	int m_errinfoline;
	int m_errinfochar;
	String m_rootpath;
	String m_templatename;
	bool m_opt_printtree;
	bool m_opt_printdocs;
	String m_templatedir;
	String m_modpaths;
	String m_docpath;
	c_DocDecl* m_rootdecl;
	c_DocDecl* m_rootmodules;
	c_DocDecl* m_rootdocs;
	c_DocDecl* m_branch3rdparty;
	c_ApiDoccer* m_apidoccer;
	c_DocDoccer* m_docdoccer;
	c_StringMap3* m_indexes;
	c_DocDecl* m_curdecl;
	String m_template;
	c_PageMaker* m_pager;
	c_Markdown* m_marker;
	Array<String > m_iconlinks_url;
	Array<String > m_iconlinks_icon;
	c_IntStack* m_blocks;
	c_Makedocs();
	void p_Message(String,String);
	void p_Die(String);
	void p_ParseAppArgs();
	int p__IsInCxRoot();
	void p_BrowseToCxRoot();
	void p_LoadTemplateName();
	void p_Warning(String);
	void p_LoadModulesPath();
	void p_LoadDocPath();
	void p_CleanUpEarly();
	void p_CopyFile(String,String);
	void p__AssertPath(String);
	void p_CopyDir(String,String,bool);
	void p_CopyTemplateData();
	void p_SetErrInfoFile(String);
	void p_SetErrInfoLine(int);
	void p_SetErrInfoChar(int);
	void p_Error(String);
	void p_ClearErrInfo();
	void p__CopyExamples(c_DocDecl*,c_DocDeclStack*);
	void p__AssignExamples(c_DocDecl*);
	void p__CompleteIdentifiers(c_DocDecl*,String,String);
	void p__CompleteImports();
	void p__CompleteInherited(c_DocDecl*,c_DocDecl*);
	void p__CompleteExtends();
	String p__SummaryFromDescription(String);
	void p__CompleteSummaries();
	void p__CompleteExamples();
	void p_Complete();
	void p__MoveToIndex(String,c_DocDeclStack*);
	void p__CreateIndex(String,c_DocDeclStack*,String);
	void p__FillMasterIndex(c_DocDecl*,c_DocDeclStack*);
	void p_CreateIndexes();
	String p_BuildDocLink(c_DocDecl*,c_DocDecl*);
	void p_WriteDeclFiles();
	static int m_resolvecode;
	String p_ResolveDocLink(String);
	String p__ApplyPageTemplate(Array<String >,Array<String >,String);
	void p__WritePage(String,String);
	Array<String > p__LinkUrlsFromIdents(Array<String >);
	void p_WriteDocs();
	void p_WriteExamples();
	c_Makedocs* m_new();
	String p_ResolveLink(String,String);
	String p_BeginPrettyBlock();
	String p_EndPrettyBlock();
	String p_PrettifyLine(String);
	void mark();
};
class c_Stack : public Object{
	public:
	Array<String > m_data;
	int m_length;
	c_Stack();
	c_Stack* m_new();
	c_Stack* m_new2(Array<String >);
	void p_Push(String);
	void p_Push2(Array<String >,int,int);
	void p_Push3(Array<String >,int);
	virtual bool p_Equals(String,String);
	bool p_Contains(String);
	bool p_IsEmpty();
	String p_Get(int);
	static String m_NIL;
	String p_Pop();
	Array<String > p_ToArray();
	void mark();
};
class c_StringStack : public c_Stack{
	public:
	c_StringStack();
	c_StringStack* m_new(Array<String >);
	c_StringStack* m_new2();
	String p_Join(String);
	bool p_Equals(String,String);
	void mark();
};
String bb_os_ExtractDir(String);
class c_List : public Object{
	public:
	c_Node* m__head;
	c_List();
	c_List* m_new();
	c_Node* p_AddLast(String);
	c_List* m_new2(Array<String >);
	bool p_IsEmpty();
	String p_RemoveFirst();
	virtual bool p_Equals(String,String);
	c_Node* p_Find(String,c_Node*);
	c_Node* p_Find2(String);
	void p_RemoveFirst2(String);
	int p_Count();
	c_Enumerator* p_ObjectEnumerator();
	Array<String > p_ToArray();
	void mark();
};
class c_StringList : public c_List{
	public:
	c_StringList();
	c_StringList* m_new(Array<String >);
	c_StringList* m_new2();
	bool p_Equals(String,String);
	void mark();
};
class c_Node : public Object{
	public:
	c_Node* m__succ;
	c_Node* m__pred;
	String m__data;
	c_Node();
	c_Node* m_new(c_Node*,c_Node*,String);
	c_Node* m_new2();
	int p_Remove();
	void mark();
};
class c_HeadNode : public c_Node{
	public:
	c_HeadNode();
	c_HeadNode* m_new();
	void mark();
};
class c_Enumerator : public Object{
	public:
	c_List* m__list;
	c_Node* m__curr;
	c_Enumerator();
	c_Enumerator* m_new(c_List*);
	c_Enumerator* m_new2();
	bool p_HasNext();
	String p_NextObject();
	void mark();
};
Array<String > bb_os_LoadDir(String,bool,bool);
int bb_os_DeleteDir(String,bool);
int bb_os_CopyDir(String,String,bool,bool);
class c_DocDecl : public Object{
	public:
	c_DocDecl* m_parent;
	int m_kind;
	String m_ident;
	int m_uid;
	bool m_canbefound;
	c_DocDeclStack* m_childs;
	int m__lastsid;
	c_DocDecl* m_target;
	c_DocDecl();
	static int m__uid;
	bool p_Canbefound();
	c_DocDecl* m_new(int,String);
	c_DocDecl* m_new2();
	void p_Add(c_DocDecl*);
	void p_Add2(c_Stack2*);
	c_DocDecl* p_GetChild(int);
	c_DocDecl* p_GetChild2(String);
	c_DocDecl* p_GetChild3(String,int);
	c_DocDeclStack* p_GetChilds(String,bool);
	c_DocDeclStack* p_GetChilds2(int,bool);
	c_DocDecl* p_GetScope(int);
	static c_DocDecl* m__root;
	static c_DocDecl* m__modroot;
	static c_DocDeclStack* m__modules;
	static c_DocDecl* m__docroot;
	static c_DocDeclStack* m__documents;
	static c_StringStack* m__primnames;
	void p_PrepareFinder();
	String p_GetType(bool,bool);
	String p_GetUniqueIdent();
	static c_StringMap2* m__primlinks;
	static int m__searchid;
	c_DocDecl* p__FindInHere(Array<String >,int,String,int);
	c_DocDecl* p__FoundDecl(c_DocDecl*,bool);
	c_DocDecl* p_FindFromHere(String,int);
	String p_GetIdent();
	String p_GetIdentWithParams();
	String p_GetTextOfChild(int);
	void p_Sort();
	String p_GetKindName();
	String p_GetScopeIdent();
	String p_GetDocType(bool);
	c_DocDecl* p__FindChild(String,String,int);
	c_DocDecl* p_FindChild(String,int);
	String p_GetGenType();
	String p_GetDocXType();
	String p_GetTargetIdent();
	String p_ToString(int);
	void mark();
};
class c_Stack2 : public Object{
	public:
	Array<c_DocDecl* > m_data;
	int m_length;
	c_Stack2();
	c_Stack2* m_new();
	c_Stack2* m_new2(Array<c_DocDecl* >);
	void p_Push4(c_DocDecl*);
	void p_Push5(Array<c_DocDecl* >,int,int);
	void p_Push6(Array<c_DocDecl* >,int);
	c_Enumerator2* p_ObjectEnumerator();
	static c_DocDecl* m_NIL;
	void p_Length(int);
	int p_Length2();
	Array<c_DocDecl* > p_ToArray();
	bool p_Equals2(c_DocDecl*,c_DocDecl*);
	void p_RemoveEach(c_DocDecl*);
	c_DocDecl* p_Get(int);
	virtual int p_Compare(c_DocDecl*,c_DocDecl*);
	bool p__Less(int,int,int);
	void p__Swap(int,int);
	bool p__Less2(int,c_DocDecl*,int);
	bool p__Less3(c_DocDecl*,int,int);
	void p__Sort(int,int,int);
	virtual void p_Sort2(bool);
	void mark();
};
class c_DocDeclStack : public c_Stack2{
	public:
	c_DocDeclStack();
	c_DocDeclStack* m_new();
	void p_Sort2(bool);
	int p_Compare(c_DocDecl*,c_DocDecl*);
	void mark();
};
class c_Enumerator2 : public Object{
	public:
	c_Stack2* m_stack;
	int m_index;
	c_Enumerator2();
	c_Enumerator2* m_new(c_Stack2*);
	c_Enumerator2* m_new2();
	bool p_HasNext();
	c_DocDecl* p_NextObject();
	void mark();
};
int bb_math_Max(int,int);
Float bb_math_Max2(Float,Float);
class c_ApiDoccer : public Object{
	public:
	c_Makedocs* m_maker;
	int m_cursect;
	c_DocDecl* m_curparagraph;
	c_Parser* m_parser;
	c_ApiDoccer();
	c_ApiDoccer* m_new(c_Makedocs*);
	c_ApiDoccer* m_new2();
	void p_LoadExamples(String,c_DocDecl*);
	void p_AppendDocContents(c_DocDecl*,String);
	void p_Error(String);
	void p_ParseDocFile(String,c_DocDecl*,String);
	void p_CopyDocsData(String,String);
	void p_ParseSourceFile(String,c_DocDecl*,String);
	void p_ParseIn(String,c_DocDecl*,String);
	void p_Parse();
	String p_StripParagraph(String);
	void p_SetPagerStrings(c_PageMaker*,c_DocDecl*,bool);
	void p_SetPagerList(c_PageMaker*,c_DocDecl*,int,String);
	void p_SetPagerLists(c_PageMaker*,c_DocDecl*,Array<int >,Array<String >);
	String p_ApplyModuleTemplate(c_DocDecl*,c_PageMaker*);
	String p_ApplyClassTemplate(c_DocDecl*,c_PageMaker*);
	void mark();
};
String bb_os_ExtractExt(String);
String bb_os_StripExt(String);
String bb_os_StripDir(String);
class c_Toker : public Object{
	public:
	String m__path;
	int m__line;
	String m__source;
	int m__length;
	String m__toke;
	int m__tokeType;
	int m__tokePos;
	c_Toker();
	static c_StringSet* m__keywords;
	static c_StringSet* m__symbols;
	int p__init();
	c_Toker* m_new(String,String);
	c_Toker* m_new2(c_Toker*);
	c_Toker* m_new3();
	int p_TCHR(int);
	String p_TSTR(int);
	String p_NextToke();
	void mark();
};
class c_Parser : public c_Toker{
	public:
	c_Parser();
	c_Parser* m_new(String);
	c_Parser* m_new2();
	c_Toker* p_Store();
	String p_NextSpace();
	String p_NextCdata(String);
	void p_Pop();
	void p_Error(String);
	void p_PopSpace(bool);
	void p_Restore(c_Toker*);
	bool p__PopToken(String,int,bool,bool,bool);
	bool p_PopUntilToken(String,bool);
	bool p_PopUntilToken2(int,bool);
	bool p_PopToken(String,bool);
	bool p_PopToken2(int,bool);
	String p_SParseModpath(bool);
	c_DocDecl* p_ParseModuleHeader(c_DocDecl*);
	c_DocDecl* p_ParseImportDecl(c_DocDecl*);
	c_DocDecl* p_GetModuleScope(c_DocDecl*);
	String p_SParseClasspath();
	void p_PopLineBreak();
	c_Stack2* p_ParseTypeParameters(c_DocDecl*);
	bool p_PopKeyword(String,bool);
	String p_SParseExpression();
	c_DocDecl* p_ParseTypeArray(c_DocDecl*);
	c_DocDecl* p_ParseType(c_DocDecl*);
	c_Stack2* p_ParseTypeArguments(c_DocDecl*);
	c_DocDecl* p_ParseClassExtends(c_DocDecl*);
	c_Stack2* p_ParseClassImplements(c_DocDecl*,String);
	c_Stack2* p_ParseInterfaceExtends(c_DocDecl*);
	c_DocDecl* p_ParseClassDecl(c_DocDecl*,int);
	c_DocDecl* p_GetClassScope(c_DocDecl*);
	c_DocDecl* p_ParseTypeDecl(c_DocDecl*,int);
	c_DocDecl* p_ParseInitialValue(c_DocDecl*);
	c_DocDecl* p_ParseVariable(c_DocDecl*,int);
	c_Stack2* p_ParseVariableSet(c_DocDecl*,int);
	c_Stack2* p_ParseFunctionParameters(c_DocDecl*);
	c_DocDecl* p_ParseFunctionDecl(c_DocDecl*,int);
	c_DocDecl* p_ParseEnumDecl(c_DocDecl*,int);
	c_DocDecl* p_ParseDecl(c_DocDecl*);
	String p_NextRestOfLine();
	int p_GetCarretLine();
	int p_GetCarretChar();
	bool p_PopUntilKeyword(String,bool);
	void mark();
};
class c_Set : public Object{
	public:
	c_Map* m_map;
	c_Set();
	c_Set* m_new(c_Map*);
	c_Set* m_new2();
	int p_Insert(String);
	bool p_Contains(String);
	void mark();
};
class c_StringSet : public c_Set{
	public:
	c_StringSet();
	c_StringSet* m_new();
	void mark();
};
class c_Map : public Object{
	public:
	c_Node2* m_root;
	c_Map();
	c_Map* m_new();
	virtual int p_Compare2(String,String)=0;
	int p_RotateLeft(c_Node2*);
	int p_RotateRight(c_Node2*);
	int p_InsertFixup(c_Node2*);
	bool p_Set(String,Object*);
	bool p_Insert2(String,Object*);
	c_Node2* p_FindNode(String);
	bool p_Contains(String);
	int p_Clear();
	Object* p_Get2(String);
	void mark();
};
class c_StringMap : public c_Map{
	public:
	c_StringMap();
	c_StringMap* m_new();
	int p_Compare2(String,String);
	void mark();
};
class c_Node2 : public Object{
	public:
	String m_key;
	c_Node2* m_right;
	c_Node2* m_left;
	Object* m_value;
	int m_color;
	c_Node2* m_parent;
	c_Node2();
	c_Node2* m_new(String,Object*,int,c_Node2*);
	c_Node2* m_new2();
	void mark();
};
bool bb_stringutil_IsSpace(int);
bool bb_stringutil_IsAlpha(int);
bool bb_stringutil_IsDigit(int);
bool bb_stringutil_IsBinDigit(int);
bool bb_stringutil_IsHexDigit(int);
class c_ThrowableString : public ThrowableObject{
	public:
	String m_str;
	c_ThrowableString();
	c_ThrowableString* m_new(String);
	c_ThrowableString* m_new2();
	String p_ToString2();
	void mark();
};
String bb_stringutil_UnifyLineEndings(String);
class c_Stack3 : public Object{
	public:
	Array<int > m_data;
	int m_length;
	c_Stack3();
	c_Stack3* m_new();
	c_Stack3* m_new2(Array<int >);
	c_BackwardsStack* p_Backwards();
	void p_Push7(int);
	void p_Push8(Array<int >,int,int);
	void p_Push9(Array<int >,int);
	static int m_NIL;
	int p_Pop();
	void p_Length(int);
	int p_Length2();
	int p_Get(int);
	void p_Clear();
	void mark();
};
class c_IntStack : public c_Stack3{
	public:
	c_IntStack();
	c_IntStack* m_new(Array<int >);
	c_IntStack* m_new2();
	void mark();
};
class c_BackwardsStack : public Object{
	public:
	c_Stack3* m_stack;
	c_BackwardsStack();
	c_BackwardsStack* m_new(c_Stack3*);
	c_BackwardsStack* m_new2();
	c_BackwardsEnumerator* p_ObjectEnumerator();
	void mark();
};
class c_BackwardsEnumerator : public Object{
	public:
	c_Stack3* m_stack;
	int m_index;
	c_BackwardsEnumerator();
	c_BackwardsEnumerator* m_new(c_Stack3*);
	c_BackwardsEnumerator* m_new2();
	bool p_HasNext();
	int p_NextObject();
	void mark();
};
class c_DocDoccer : public Object{
	public:
	c_Makedocs* m_maker;
	c_DocDoccer();
	c_DocDoccer* m_new(c_Makedocs*);
	c_DocDoccer* m_new2();
	void p_DocIn(String,c_DocDecl*,String,bool);
	void p_Doc();
	void p_Doc3rdParty();
	void mark();
};
class c_Map2 : public Object{
	public:
	c_Node3* m_root;
	c_Map2();
	c_Map2* m_new();
	virtual int p_Compare2(String,String)=0;
	c_Node3* p_FindNode(String);
	c_DocDecl* p_Get2(String);
	int p_RotateLeft2(c_Node3*);
	int p_RotateRight2(c_Node3*);
	int p_InsertFixup2(c_Node3*);
	bool p_Add3(String,c_DocDecl*);
	c_Node3* p_FirstNode();
	c_NodeEnumerator3* p_ObjectEnumerator();
	void mark();
};
class c_StringMap2 : public c_Map2{
	public:
	c_StringMap2();
	c_StringMap2* m_new();
	int p_Compare2(String,String);
	void mark();
};
class c_Node3 : public Object{
	public:
	String m_key;
	c_Node3* m_right;
	c_Node3* m_left;
	c_DocDecl* m_value;
	int m_color;
	c_Node3* m_parent;
	c_Node3();
	c_Node3* m_new(String,c_DocDecl*,int,c_Node3*);
	c_Node3* m_new2();
	c_Node3* p_NextNode();
	c_DocDecl* p_Value();
	void mark();
};
class c_Map3 : public Object{
	public:
	c_Node4* m_root;
	c_Map3();
	c_Map3* m_new();
	virtual int p_Compare2(String,String)=0;
	int p_RotateLeft3(c_Node4*);
	int p_RotateRight3(c_Node4*);
	int p_InsertFixup3(c_Node4*);
	bool p_Add4(String,c_StringMap2*);
	c_Node4* p_FirstNode();
	c_NodeEnumerator2* p_ObjectEnumerator();
	void mark();
};
class c_StringMap3 : public c_Map3{
	public:
	c_StringMap3();
	c_StringMap3* m_new();
	int p_Compare2(String,String);
	void mark();
};
class c_Node4 : public Object{
	public:
	String m_key;
	c_Node4* m_right;
	c_Node4* m_left;
	c_StringMap2* m_value;
	int m_color;
	c_Node4* m_parent;
	c_Node4();
	c_Node4* m_new(String,c_StringMap2*,int,c_Node4*);
	c_Node4* m_new2();
	c_Node4* p_NextNode();
	String p_Key();
	c_StringMap2* p_Value();
	void mark();
};
class c_Map4 : public Object{
	public:
	c_Node5* m_root;
	c_Map4();
	c_Map4* m_new();
	virtual int p_Compare2(String,String)=0;
	c_Node5* p_FindNode(String);
	bool p_Contains(String);
	int p_RotateLeft4(c_Node5*);
	int p_RotateRight4(c_Node5*);
	int p_InsertFixup4(c_Node5*);
	bool p_Add5(String,String);
	bool p_Set2(String,String);
	c_Node5* p_FirstNode();
	c_NodeEnumerator* p_ObjectEnumerator();
	void mark();
};
class c_StringMap4 : public c_Map4{
	public:
	c_StringMap4();
	c_StringMap4* m_new();
	int p_Compare2(String,String);
	void mark();
};
class c_Node5 : public Object{
	public:
	String m_key;
	c_Node5* m_right;
	c_Node5* m_left;
	String m_value;
	int m_color;
	c_Node5* m_parent;
	c_Node5();
	c_Node5* m_new(String,String,int,c_Node5*);
	c_Node5* m_new2();
	c_Node5* p_NextNode();
	String p_Key();
	String p_Value();
	void mark();
};
class c_NodeEnumerator : public Object{
	public:
	c_Node5* m_node;
	c_NodeEnumerator();
	c_NodeEnumerator* m_new(c_Node5*);
	c_NodeEnumerator* m_new2();
	bool p_HasNext();
	c_Node5* p_NextObject();
	void mark();
};
class c_PageMaker : public Object{
	public:
	String m__template;
	c_StringMap* m__decls;
	c_Stack4* m__scopes;
	c_Stack5* m__lists;
	c_IntStack* m__iters;
	c_PageMaker();
	c_PageMaker* m_new(String);
	c_PageMaker* m_new2();
	void p_Clear();
	void p_SetString(String,String);
	void p_BeginList(String);
	void p_AddItem();
	void p_EndList();
	Object* p_GetValue(String);
	String p_GetString(String);
	c_Stack4* p_GetList(String);
	String p_MakePage();
	void mark();
};
class c_Stack4 : public Object{
	public:
	Array<c_StringMap* > m_data;
	int m_length;
	c_Stack4();
	c_Stack4* m_new();
	c_Stack4* m_new2(Array<c_StringMap* >);
	void p_Push10(c_StringMap*);
	void p_Push11(Array<c_StringMap* >,int,int);
	void p_Push12(Array<c_StringMap* >,int);
	static c_StringMap* m_NIL;
	void p_Clear();
	c_StringMap* p_Top();
	c_StringMap* p_Pop();
	void p_Length(int);
	int p_Length2();
	c_StringMap* p_Get(int);
	void mark();
};
class c_Markdown : public Object{
	public:
	c_ILinkResolver* m__resolver;
	c_IPrettifier* m__prettifier;
	String m__blk;
	c_Markdown();
	c_Markdown* m_new(c_ILinkResolver*,c_IPrettifier*);
	c_Markdown* m_new2();
	String p_Prettify(String);
	String p_SetBlock(String);
	int p_Find3(String,String,int);
	String p_ReplaceSpanTags(String,String,String);
	String p_ReplacePrefixTags(String,String,String);
	String p_ReplaceLinks(String);
	String p_ReplaceEscs(String);
	String p_SpanToHtml(String);
	String p_LineToHtml(String);
	String p_ToHtml(String);
	void mark();
};
class c_Stack5 : public Object{
	public:
	Array<c_Stack4* > m_data;
	int m_length;
	c_Stack5();
	c_Stack5* m_new();
	c_Stack5* m_new2(Array<c_Stack4* >);
	static c_Stack4* m_NIL;
	void p_Clear();
	void p_Push13(c_Stack4*);
	void p_Push14(Array<c_Stack4* >,int,int);
	void p_Push15(Array<c_Stack4* >,int);
	c_Stack4* p_Top();
	c_Stack4* p_Pop();
	void mark();
};
class c_StringObject : public Object{
	public:
	String m_value;
	c_StringObject();
	c_StringObject* m_new(int);
	c_StringObject* m_new2(Float);
	c_StringObject* m_new3(String);
	c_StringObject* m_new4();
	void mark();
};
String bb_stringutil_HtmlEscape(String);
int bb_math_Min(int,int);
Float bb_math_Min2(Float,Float);
class c_NodeEnumerator2 : public Object{
	public:
	c_Node4* m_node;
	c_NodeEnumerator2();
	c_NodeEnumerator2* m_new(c_Node4*);
	c_NodeEnumerator2* m_new2();
	bool p_HasNext();
	c_Node4* p_NextObject();
	void mark();
};
class c_NodeEnumerator3 : public Object{
	public:
	c_Node3* m_node;
	c_NodeEnumerator3();
	c_NodeEnumerator3* m_new(c_Node3*);
	c_NodeEnumerator3* m_new2();
	bool p_HasNext();
	c_Node3* p_NextObject();
	void mark();
};
int bbMain();
c_Makedocs::c_Makedocs(){
	m_ignoremods=(new c_StringStack)->m_new2();
	m_errinfofile=String();
	m_errinfoline=0;
	m_errinfochar=0;
	m_rootpath=String();
	m_templatename=String();
	m_opt_printtree=false;
	m_opt_printdocs=false;
	m_templatedir=String();
	m_modpaths=String();
	m_docpath=String();
	m_rootdecl=0;
	m_rootmodules=0;
	m_rootdocs=0;
	m_branch3rdparty=0;
	m_apidoccer=0;
	m_docdoccer=0;
	m_indexes=0;
	m_curdecl=0;
	m_template=String();
	m_pager=0;
	m_marker=0;
	m_iconlinks_url=Array<String >();
	m_iconlinks_icon=Array<String >();
	m_blocks=0;
}
void c_Makedocs::p_Message(String t_pType,String t_pPrompt){
	String t_msg=String();
	if((m_errinfofile).Length()!=0){
		String t_errinfo=m_errinfofile;
		if((m_errinfoline)!=0){
			t_errinfo=t_errinfo+(String(L", line ",7)+String(m_errinfoline));
			if((m_errinfochar)!=0){
				t_errinfo=t_errinfo+(String(L", char ",7)+String(m_errinfochar));
			}
		}
		t_msg=String(L"[",1)+t_pType+String(L" (",2)+t_errinfo+String(L")] ",3)+t_pPrompt;
	}else{
		t_msg=String(L"[",1)+t_pType+String(L"] ",2)+t_pPrompt;
	}
	bbPrint(t_msg);
}
void c_Makedocs::p_Die(String t_pPrompt){
	p_Message(String(L"FATAL ERROR",11),t_pPrompt);
	ExitApp(0);
}
void c_Makedocs::p_ParseAppArgs(){
	int t_i=1;
	while(t_i<AppArgs().Length()){
		String t_arg=AppArgs()[t_i];
		t_i+=1;
		String t_1=t_arg;
		if(t_1==String(L"-ignore",7)){
			if(t_i<AppArgs().Length()){
				String t_ignparam=AppArgs()[t_i];
				Array<String > t_ignored=t_ignparam.Split(String(L";",1));
				Array<String > t_=t_ignored;
				int t_2=0;
				while(t_2<t_.Length()){
					String t_ign=t_[t_2];
					t_2=t_2+1;
					m_ignoremods->p_Push(t_ign);
				}
				t_i+=1;
			}else{
				p_Die(String(L"-ignore must be followed by module name",39));
			}
		}else{
			if(t_1==String(L"-path",5)){
				if(t_i<AppArgs().Length()){
					m_rootpath=AppArgs()[t_i];
					t_i+=1;
					if(m_rootpath.EndsWith(String(L"/",1)) || m_rootpath.EndsWith(String(L"\\",1))){
						m_rootpath=m_rootpath.Slice(0,-1);
					}
				}else{
					p_Die(String(L"-path must be followed by path",30));
				}
			}else{
				if(t_1==String(L"-template",9)){
					if(t_i<AppArgs().Length()){
						m_templatename=AppArgs()[t_i];
						t_i+=1;
					}else{
						p_Die(String(L"-template must be followed by template name",43));
					}
				}else{
					if(t_1==String(L"-printtree",10)){
						m_opt_printtree=true;
					}else{
						if(t_1==String(L"-printdocs",10)){
							m_opt_printdocs=true;
						}else{
							p_Die(String(L"invalid argument: ",18)+t_arg);
						}
					}
				}
			}
		}
	}
}
int c_Makedocs::p__IsInCxRoot(){
	if(FileType(String(L"docs",4))==2 && FileType(String(L"bin",3))==2 && FileType(String(L"modules",7))==2){
		return 1;
	}
	return 0;
}
void c_Makedocs::p_BrowseToCxRoot(){
	if(m_rootpath==String()){
		ChangeDir(bb_os_ExtractDir(AppPath()));
		while(!((p__IsInCxRoot())!=0)){
			String t_odir=CurrentDir();
			ChangeDir(String(L"..",2));
			m_rootpath=CurrentDir();
			if(t_odir==m_rootpath){
				p_Die(String(L"Cerberus X root path not found",30));
			}
		}
	}else{
		if(FileType(m_rootpath)!=2){
			p_Die(String(L"path '",6)+m_rootpath+String(L"' does not exist",16));
		}
		ChangeDir(m_rootpath);
		if(!((p__IsInCxRoot())!=0)){
			p_Die(m_rootpath+String(L" is not a valid Cerberus X root path",36));
		}
	}
	bbPrint(String(L"Making docs for ",16)+CurrentDir());
}
void c_Makedocs::p_LoadTemplateName(){
	if(m_templatename==String()){
		m_templatename=LoadString(String(L"bin/docstyle.txt",16)).Trim();
	}
	m_templatedir=String(L"docs/templates/",15)+m_templatename;
	if(m_templatename==String() || FileType(m_templatedir)!=2){
		p_Die(String(L"specified template does not exist: ",35)+m_templatename);
	}
}
void c_Makedocs::p_Warning(String t_pPrompt){
	p_Message(String(L"WARNING",7),t_pPrompt);
}
void c_Makedocs::p_LoadModulesPath(){
	m_modpaths=GetEnv(String(L"MODPATH",7));
	if(m_modpaths==String()){
		String t_config=LoadString(String(L"bin/config.",11)+HostOS()+String(L".txt",4));
		if(!((t_config).Length()!=0)){
			p_Warning(String(L"config file not found",21));
		}
		Array<String > t_=t_config.Split(String(L"\n",1));
		int t_2=0;
		while(t_2<t_.Length()){
			String t_line=t_[t_2];
			t_2=t_2+1;
			t_line=t_line.Trim();
			if(t_line.StartsWith(String(L"'",1))){
				continue;
			}
			Array<String > t_bits=t_line.Split(String(L"=",1));
			if(t_bits.Length()!=2){
				continue;
			}
			if(t_bits[0]==String(L"MODPATH",7)){
				String t_val=t_bits[1];
				do{
					int t_i0=t_val.Find(String(L"${",2),0);
					int t_i1=t_val.Find(String(L"}",1),0);
					if(t_i0==-1 || t_i1==-1){
						break;
					}
					String t_ident=t_val.Slice(t_i0+2,t_i1);
					String t_3=t_ident;
					if(t_3==String(L"MONKEYDIR",9) || t_3==String(L"CERBERUSDIR",11)){
						t_ident=CurrentDir();
					}else{
						t_ident=GetEnv(t_ident);
					}
					t_val=t_val.Slice(0,t_i0)+t_ident+t_val.Slice(t_i1+1);
				}while(!(false));
				if(t_val.StartsWith(String(L"\"",1)) && t_val.EndsWith(String(L"\"",1))){
					t_val=t_val.Slice(1,-1);
				}
				m_modpaths=t_val;
				if(!((m_modpaths).Length()!=0)){
					p_Warning(String(L"MODPATH not set",15));
				}
				break;
			}
		}
	}
	m_modpaths=m_modpaths.Replace(String(L"\\",1),String(L"/",1));
	m_modpaths=m_modpaths.Replace(String(L"|",1),String(L";",1));
}
void c_Makedocs::p_LoadDocPath(){
	m_docpath=CurrentDir()+String(L"/",1)+String(L"docs/cerberusdoc",16);
}
void c_Makedocs::p_CleanUpEarly(){
	if(!m_opt_printdocs && !m_opt_printtree){
		bb_os_DeleteDir(String(L"docs/html",9),true);
		CreateDir(String(L"docs/html",9));
		CreateDir(String(L"docs/html/data",14));
		CreateDir(String(L"docs/html/examples",18));
	}
}
void c_Makedocs::p_CopyFile(String t_pSrc,String t_pDst){
	if(!m_opt_printdocs && !m_opt_printtree){
		CopyFile(t_pSrc,t_pDst);
	}
}
void c_Makedocs::p__AssertPath(String t_pDst){
	int t_p=-1;
	String t_path=String();
	do{
		t_p=t_pDst.Find(String(L"/",1),t_p+1);
		if(t_p==-1){
			t_path=t_pDst;
		}else{
			t_path=t_pDst.Slice(0,t_p);
		}
		if(FileType(t_path)!=2){
			CreateDir(t_path);
		}
	}while(!(t_p==-1));
}
void c_Makedocs::p_CopyDir(String t_pSrc,String t_pDst,bool t_pRecursive){
	if(!m_opt_printdocs && !m_opt_printtree){
		p__AssertPath(t_pDst);
		bb_os_CopyDir(t_pSrc,t_pDst,t_pRecursive,false);
	}
}
void c_Makedocs::p_CopyTemplateData(){
	Array<String > t_=LoadDir(m_templatedir);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_file=t_[t_2];
		t_2=t_2+1;
		String t_path=m_templatedir+String(L"/",1)+t_file;
		int t_22=FileType(t_path);
		if(t_22==1){
			if(!t_file.EndsWith(String(L"_template.html",14))){
				p_CopyFile(t_path,String(L"docs/html/",10)+t_file);
			}
		}else{
			if(t_22==2){
				if(t_file==String(L"data",4)){
					p_CopyDir(t_path,String(L"docs/html/data",14),true);
				}
			}
		}
	}
}
void c_Makedocs::p_SetErrInfoFile(String t_pInfo){
	m_errinfofile=t_pInfo;
}
void c_Makedocs::p_SetErrInfoLine(int t_pLine){
	m_errinfoline=t_pLine;
}
void c_Makedocs::p_SetErrInfoChar(int t_pChar){
	m_errinfochar=t_pChar;
}
void c_Makedocs::p_Error(String t_pPrompt){
	p_Message(String(L"ERROR",5),t_pPrompt);
}
void c_Makedocs::p_ClearErrInfo(){
	m_errinfofile=String();
	m_errinfoline=0;
	m_errinfochar=0;
}
void c_Makedocs::p__CopyExamples(c_DocDecl* t_pScope,c_DocDeclStack* t_pExamples){
	c_Enumerator2* t_=t_pExamples->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_DocDecl* t_ex=t_->p_NextObject();
		String t_source=t_ex->p_GetChild(221)->m_ident;
		t_source=String(L"<pre>",5)+t_source+String(L"</pre>",6);
		c_DocDeclStack* t_decls=t_pScope->p_GetChilds(t_ex->m_ident,true);
		if((t_decls)!=0){
			c_Enumerator2* t_2=t_decls->p_ObjectEnumerator();
			while(t_2->p_HasNext()){
				c_DocDecl* t_d=t_2->p_NextObject();
				int t_4=t_d->m_kind;
				if(t_4==901 || t_4==301 || t_4==302 || t_4==310 || t_4==320 || t_4==321 || t_4==403 || t_4==405 || t_4==406 || t_4==410 || t_4==420 || t_4==421 || t_4==423){
					if(!((t_d->p_GetChild(801))!=0)){
						t_d->p_Add((new c_DocDecl)->m_new(801,t_source));
					}
				}else{
					if(t_4==601){
						if(!((t_d->m_parent->p_GetChild(801))!=0)){
							t_d->m_parent->p_Add((new c_DocDecl)->m_new(801,t_source));
						}
					}
				}
			}
		}
	}
}
void c_Makedocs::p__AssignExamples(c_DocDecl* t_pScope){
	if((t_pScope->m_childs)!=0){
		c_Enumerator2* t_=t_pScope->m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_c->m_kind==901 || t_c->m_kind==900){
				c_DocDeclStack* t_examples=t_c->p_GetChilds2(220,false);
				if((t_examples)!=0){
					p__CopyExamples(t_c,t_examples);
				}
				p__AssignExamples(t_c);
			}
		}
	}
}
void c_Makedocs::p__CompleteIdentifiers(c_DocDecl* t_pDecl,String t_pScope,String t_pModPath){
	String t_uident=String();
	if((t_pScope).Length()!=0){
		t_uident=t_pScope+String(L".",1);
	}
	if((t_pModPath).Length()!=0){
		t_uident=t_pModPath+String(L".",1)+t_uident;
	}
	bool t_completechilds=false;
	int t_5=t_pDecl->m_kind;
	if(t_5==310 || t_5==403 || t_5==406 || t_5==405 || t_5==410){
		t_pDecl->p_Add((new c_DocDecl)->m_new(201,t_uident));
		String t_txt=String();
		c_DocDeclStack* t_params=t_pDecl->p_GetChilds2(600,false);
		if((t_params)!=0){
			c_Enumerator2* t_=t_params->p_ObjectEnumerator();
			while(t_->p_HasNext()){
				c_DocDecl* t_p=t_->p_NextObject();
				if((t_txt).Length()!=0){
					t_txt=t_txt+String(L",",1);
				}
				t_txt=t_txt+t_p->p_GetType(false,true);
			}
		}
		t_pDecl->p_Add((new c_DocDecl)->m_new(202,String(L"(",1)+t_txt+String(L")",1)));
	}else{
		if(t_5==901 || t_5==900){
			t_pModPath=t_pDecl->p_GetUniqueIdent();
			t_completechilds=true;
		}else{
			if(t_5==301 || t_5==302){
				t_pDecl->p_Add((new c_DocDecl)->m_new(201,t_uident));
				t_pScope=t_pDecl->m_ident;
				t_completechilds=true;
			}else{
				if(t_5==100 || t_5==102 || t_5==101){
					t_completechilds=true;
				}else{
					if(t_5==320 || t_5==322 || t_5==321 || t_5==420 || t_5==422 || t_5==421 || t_5==423){
						t_pDecl->p_Add((new c_DocDecl)->m_new(201,t_uident));
					}
				}
			}
		}
	}
	if(((t_pDecl->m_childs)!=0) && t_completechilds){
		c_Enumerator2* t_2=t_pDecl->m_childs->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_DocDecl* t_c=t_2->p_NextObject();
			p__CompleteIdentifiers(t_c,t_pScope,t_pModPath);
		}
	}
}
void c_Makedocs::p__CompleteImports(){
	c_DocDeclStack* t_imps=m_rootmodules->p_GetChilds2(300,true);
	if((t_imps)!=0){
		c_Enumerator2* t_=t_imps->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_i=t_->p_NextObject();
			c_DocDecl* t_decl=0;
			t_decl=m_rootmodules->p_FindFromHere(t_i->m_ident,901);
			if((t_decl)!=0){
				t_i->m_target=t_decl;
				c_DocDecl* t_d=(new c_DocDecl)->m_new(700,t_i->m_parent->p_GetUniqueIdent());
				t_d->m_target=t_i->m_parent;
				t_decl->p_Add(t_d);
			}
		}
	}
}
void c_Makedocs::p__CompleteInherited(c_DocDecl* t_pFrom,c_DocDecl* t_pTo){
	if(!((t_pFrom->p_GetChild(499))!=0)){
		t_pFrom->p_Add((new c_DocDecl)->m_new(499,String()));
		c_DocDeclStack* t_exts=t_pFrom->p_GetChilds2(401,false);
		if((t_exts)!=0){
			c_Enumerator2* t_=t_exts->p_ObjectEnumerator();
			while(t_->p_HasNext()){
				c_DocDecl* t_e=t_->p_NextObject();
				p__CompleteInherited(t_e,t_pFrom);
			}
		}
	}
	if((t_pFrom->m_childs)!=0){
		c_Enumerator2* t_2=t_pFrom->m_childs->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_DocDecl* t_c=t_2->p_NextObject();
			int t_kind=0;
			int t_6=t_c->m_kind;
			if(t_6==406){
				t_kind=456;
			}else{
				if(t_6==403){
					t_kind=453;
				}else{
					if(t_6==405){
						t_kind=455;
					}else{
						if(t_6==410){
							t_kind=460;
						}else{
							if(t_6==420){
								t_kind=470;
							}else{
								if(t_6==422){
									t_kind=472;
								}else{
									if(t_6==421){
										t_kind=471;
									}else{
										if(t_6==423){
											t_kind=473;
										}
									}
								}
							}
						}
					}
				}
			}
			if(((t_kind)!=0) && !((t_pTo->p_GetChild3(t_c->m_ident,t_c->m_kind))!=0)){
				c_DocDecl* t_decl=(new c_DocDecl)->m_new(t_kind,t_c->m_ident);
				t_decl->m_target=t_c;
				t_pTo->p_Add(t_decl);
			}
		}
	}
}
void c_Makedocs::p__CompleteExtends(){
	c_DocDeclStack* t_exts=m_rootmodules->p_GetChilds2(401,true);
	c_DocDeclStack* t_imps=m_rootmodules->p_GetChilds2(402,true);
	if((t_exts)!=0){
		c_Enumerator2* t_=t_exts->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_e=t_->p_NextObject();
			c_DocDecl* t_decl=0;
			t_decl=t_e->m_parent->m_parent->p_FindFromHere(t_e->m_ident,301);
			if(!((t_decl)!=0)){
				t_decl=t_e->m_parent->m_parent->p_FindFromHere(t_e->m_ident,302);
			}
			if((t_decl)!=0){
				t_e->m_target=t_decl;
				c_DocDecl* t_d=(new c_DocDecl)->m_new(701,t_e->m_parent->p_GetUniqueIdent());
				t_d->m_target=t_e->m_parent;
				t_decl->p_Add(t_d);
				p__CompleteInherited(t_decl,t_e->m_parent);
			}
		}
	}
	if((t_imps)!=0){
		c_Enumerator2* t_2=t_imps->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_DocDecl* t_i=t_2->p_NextObject();
			c_DocDecl* t_decl2=0;
			t_decl2=t_i->m_parent->m_parent->p_FindFromHere(t_i->m_ident,302);
			if((t_decl2)!=0){
				t_i->m_target=t_decl2;
				c_DocDecl* t_d2=(new c_DocDecl)->m_new(702,t_i->m_parent->p_GetUniqueIdent());
				t_d2->m_target=t_i->m_parent;
				t_decl2->p_Add(t_d2);
			}
		}
	}
}
String c_Makedocs::p__SummaryFromDescription(String t_pDescription){
	Array<String > t_lines=t_pDescription.Replace(String(L"\r",1),String(L"\n",1)).Split(String(L"\n",1));
	String t_summary=String();
	Array<String > t_=t_lines;
	int t_2=0;
	while(t_2<t_.Length()){
		String t_line=t_[t_2];
		t_2=t_2+1;
		String t_txt=t_line.Trim();
		if(((t_txt).Length()!=0) && !t_txt.StartsWith(String(L">",1))){
			int t_i=0;
			while(t_i<t_txt.Length()-1){
				int t_c=(int)t_txt[t_i];
				if(t_c==46 && bb_stringutil_IsSpace((int)t_txt[t_i+1])){
					t_i+=1;
					return t_txt.Slice(0,t_i);
				}
				t_i+=1;
			}
			return t_txt;
		}
	}
	return String();
}
void c_Makedocs::p__CompleteSummaries(){
	c_DocDeclStack* t_descs=m_rootdecl->p_GetChilds2(800,true);
	c_DocDeclStack* t_contents=m_rootdecl->p_GetChilds2(805,true);
	if(((t_descs)!=0) && ((t_contents)!=0)){
		t_descs->p_Push6(t_contents->p_ToArray(),0);
	}else{
		if((t_contents)!=0){
			t_descs=t_contents;
		}
	}
	if((t_descs)!=0){
		c_Enumerator2* t_=t_descs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			String t_summary=p__SummaryFromDescription(t_d->m_ident);
			if(t_summary.StartsWith(String(L"* ",2))){
				t_summary=t_summary.Slice(2);
			}
			if(t_summary.StartsWith(String(L"+ ",2))){
				t_summary=t_summary.Slice(2);
			}
			if(t_summary.StartsWith(String(L"| ",2))){
				t_summary=t_summary.Slice(2);
			}
			c_DocDecl* t_decl=(new c_DocDecl)->m_new(850,t_summary);
			t_d->m_parent->p_Add(t_decl);
		}
	}
}
void c_Makedocs::p__CompleteExamples(){
	c_DocDeclStack* t_examples=m_rootmodules->p_GetChilds2(801,true);
	if((t_examples)!=0){
		c_Enumerator2* t_=t_examples->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_ex=t_->p_NextObject();
			String t_str=t_ex->m_parent->p_GetUniqueIdent().Replace(String(L".",1),String(L"_",1));
			t_str=t_str+String(L".cxs",4);
			t_ex->p_Add((new c_DocDecl)->m_new(222,t_str));
		}
	}
}
void c_Makedocs::p_Complete(){
	m_rootdecl->p_PrepareFinder();
	p__AssignExamples(m_rootmodules);
	p__CompleteIdentifiers(m_rootmodules,String(),String());
	p__CompleteImports();
	p__CompleteExtends();
	p__CompleteSummaries();
	p__CompleteExamples();
}
void c_Makedocs::p__MoveToIndex(String t_pIndex,c_DocDeclStack* t_pStack){
	c_StringMap2* t_index=(new c_StringMap2)->m_new();
	if((t_pStack)!=0){
		c_Enumerator2* t_=t_pStack->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			String t_key=t_d->p_GetIdentWithParams();
			t_key=t_key+t_d->p_GetTextOfChild(201);
			t_index->p_Add3(t_key.ToLower(),t_d);
		}
	}
	m_indexes->p_Add4(t_pIndex,t_index);
}
void c_Makedocs::p__CreateIndex(String t_pIndex,c_DocDeclStack* t_pStack,String t_pSummary){
	c_DocDecl* t_decl=(new c_DocDecl)->m_new(903,t_pIndex);
	String t_txt=t_pSummary;
	if(!((t_txt).Length()!=0)){
		t_txt=t_pIndex;
	}
	t_decl->p_Add((new c_DocDecl)->m_new(850,t_txt));
	m_rootdocs->p_Add(t_decl);
	p__MoveToIndex(t_pIndex,t_pStack);
}
void c_Makedocs::p__FillMasterIndex(c_DocDecl* t_pDecl,c_DocDeclStack* t_pStack){
	if((t_pDecl->m_childs)!=0){
		c_Enumerator2* t_=t_pDecl->m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			int t_7=t_d->m_kind;
			if(t_7==902 || t_7==903 || t_7==901 || t_7==301 || t_7==302 || t_7==310 || t_7==320 || t_7==321 || t_7==403 || t_7==405 || t_7==406 || t_7==410 || t_7==420 || t_7==421 || t_7==423 || t_7==601){
				t_pStack->p_Push4(t_d);
				p__FillMasterIndex(t_d,t_pStack);
			}else{
				if(t_7==101 || t_7==102 || t_7==900 || t_7==322 || t_7==422){
					p__FillMasterIndex(t_d,t_pStack);
				}
			}
		}
	}
}
void c_Makedocs::p_CreateIndexes(){
	m_indexes=(new c_StringMap3)->m_new();
	c_DocDeclStack* t_idecls=m_rootdocs->p_GetChilds2(903,true);
	if((t_idecls)!=0){
		c_Enumerator2* t_=t_idecls->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_idx=t_->p_NextObject();
			c_DocDeclStack* t_docs=t_idx->p_GetChilds2(902,false);
			c_DocDeclStack* t_idxs=t_idx->p_GetChilds2(903,false);
			if(((t_docs)!=0) && ((t_idxs)!=0)){
				t_docs->p_Push6(t_idxs->p_ToArray(),0);
			}else{
				if((t_idxs)!=0){
					t_docs=t_idxs;
				}
			}
			p__MoveToIndex(t_idx->p_GetUniqueIdent(),t_docs);
			t_idx->p_Add((new c_DocDecl)->m_new(850,String(L"Index of ",9)+t_idx->p_GetIdent()));
		}
	}
	c_DocDeclStack* t_modules=m_rootdecl->p_GetChilds2(901,true);
	c_DocDeclStack* t_classes=m_rootdecl->p_GetChilds2(301,true);
	c_DocDeclStack* t_interfaces=m_rootdecl->p_GetChilds2(302,true);
	c_DocDeclStack* t_functions=m_rootdecl->p_GetChilds2(310,true);
	p__CreateIndex(String(L"Modules",7),t_modules,String(L"All documented modules",22));
	p__CreateIndex(String(L"Classes",7),t_classes,String(L"All documented classes",22));
	p__CreateIndex(String(L"Interfaces",10),t_interfaces,String(L"All documented interfaces",25));
	p__CreateIndex(String(L"Functions",9),t_functions,String(L"All documented functions",24));
	c_DocDeclStack* t_master=(new c_DocDeclStack)->m_new();
	p__FillMasterIndex(m_rootdecl,t_master);
	p__CreateIndex(String(L"Index",5),t_master,String(L"Everything",10));
}
String c_Makedocs::p_BuildDocLink(c_DocDecl* t_pDecl,c_DocDecl* t_pScope){
	while((t_pScope)!=0){
		int t_9=t_pScope->m_kind;
		if(t_9==301 || t_9==302 || t_9==901 || t_9==902 || t_9==903 || t_9==102 || t_9==101 || t_9==100){
			break;
		}
		t_pScope=t_pScope->m_parent;
	}
	String t_anchor=String();
	String t_document=String();
	c_DocDecl* t_bit=t_pDecl;
	do{
		int t_10=t_bit->m_kind;
		if(t_10==902 || t_10==903){
			c_DocDecl* t_pdecl2=t_bit->p_GetChild(201);
			if((t_pdecl2)!=0){
				t_document=t_pdecl2->m_ident.Replace(String(L"/",1),String(L"_",1));
			}
			t_document=t_document+t_bit->m_ident;
			t_bit=m_rootdocs;
		}else{
			if(t_10==901){
				t_document=t_bit->p_GetIdent()+t_document;
				t_document=String(L"Modules_",8)+t_document;
				t_bit=m_rootmodules;
			}else{
				if(t_10==301 || t_10==302){
					t_document=String(L"_",1)+t_bit->m_ident+t_document;
				}else{
					if(t_10==100 || t_10==101 || t_10==102){
						t_document=t_document+String(L".html",5);
						break;
					}else{
						if(t_10==322 || t_10==422 || t_10==472){
						}else{
							t_anchor=String(L"#",1)+t_bit->p_GetIdentWithParams();
							if(t_bit->m_parent==t_pScope){
								break;
							}
						}
					}
				}
			}
		}
		t_bit=t_bit->m_parent;
	}while(!(false));
	return t_document+t_anchor;
}
void c_Makedocs::p_WriteDeclFiles(){
	p_ClearErrInfo();
	m_curdecl=m_rootmodules;
	c_DocDeclStack* t_alldecls=m_rootdecl->p_GetChilds2(0,true);
	String t_txt=String();
	c_StringMap4* t_idx=(new c_StringMap4)->m_new();
	c_Enumerator2* t_=t_alldecls->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_DocDecl* t_d=t_->p_NextObject();
		int t_8=t_d->m_kind;
		if(t_8==901 || t_8==301 || t_8==302 || t_8==310 || t_8==320 || t_8==321 || t_8==403 || t_8==405 || t_8==406 || t_8==410 || t_8==420 || t_8==421 || t_8==423 || t_8==601){
			if(!t_idx->p_Contains(t_d->m_ident)){
				t_idx->p_Add5(t_d->m_ident,p_BuildDocLink(t_d,0));
			}else{
				String t_str=String(L"Index.html#",11)+t_d->m_ident;
				t_idx->p_Set2(t_d->m_ident,t_str);
			}
		}else{
			if(t_8==902){
				if(t_d->p_GetTextOfChild(201)==String(L"Programming/Keywords/",21)){
					t_idx->p_Add5(t_d->m_ident,p_BuildDocLink(t_d,0));
				}
			}
		}
	}
	c_NodeEnumerator* t_2=t_idx->p_ObjectEnumerator();
	while(t_2->p_HasNext()){
		c_Node5* t_d2=t_2->p_NextObject();
		t_txt=t_txt+(t_d2->p_Key()+String(L":",1)+t_d2->p_Value()+String(L"\n",1));
	}
	SaveString(t_txt,String(L"docs/html/index.txt",19));
	t_txt=String();
	c_Enumerator2* t_3=t_alldecls->p_ObjectEnumerator();
	while(t_3->p_HasNext()){
		c_DocDecl* t_d3=t_3->p_NextObject();
		String t_ktxt=t_d3->p_GetKindName();
		if(t_ktxt!=String(L"Unspecified",11)){
			String t_str2=t_d3->p_GetScopeIdent()+t_d3->m_ident+t_d3->p_GetDocType(false).Replace(String(L" ",1),String());
			t_txt=t_txt+(t_ktxt+String(L" ",1)+t_str2);
			t_txt=t_txt+(String(L";",1)+p_BuildDocLink(t_d3,0));
			t_txt=t_txt+String(L";",1);
			t_txt=t_txt+String(L"\n",1);
		}
	}
	SaveString(t_txt,String(L"docs/html/decls.txt",19));
}
int c_Makedocs::m_resolvecode;
String c_Makedocs::p_ResolveDocLink(String t_pLink){
	if(!((t_pLink).Length()!=0)){
		m_resolvecode=-1;
		p_Warning(String(L"empty link, please fix",22));
		return String();
	}
	String t_link=t_pLink;
	String t_anchor=String();
	if(t_link.Contains(String(L"#",1))){
		int t_p=t_link.Find(String(L"#",1),0);
		t_anchor=t_link.Slice(t_p);
		t_link=t_link.Slice(0,t_p);
	}
	c_DocDecl* t_dest=m_curdecl->p_FindFromHere(t_link,0);
	if(!((t_dest)!=0)){
		t_dest=m_rootdecl->p_FindChild(t_link,0);
	}
	if((t_dest)!=0){
		m_resolvecode=0;
		t_link=p_BuildDocLink(t_dest,m_curdecl);
	}else{
		m_resolvecode=-1;
		p_Warning(String(L"link destination not found: ",28)+t_link);
	}
	return t_link;
}
String c_Makedocs::p__ApplyPageTemplate(Array<String > t_pLinkIdents,Array<String > t_pLinkUrls,String t_pContents){
	m_pager->p_Clear();
	if((m_iconlinks_url).Length()!=0){
		m_pager->p_BeginList(String(L"ICONLINKS",9));
		for(int t_i=0;t_i<m_iconlinks_url.Length();t_i=t_i+1){
			m_pager->p_AddItem();
			m_pager->p_SetString(String(L"URL",3),m_iconlinks_url[t_i]);
			m_pager->p_SetString(String(L"ICON",4),m_iconlinks_icon[t_i]);
		}
		m_pager->p_EndList();
	}
	if((t_pLinkIdents).Length()!=0){
		m_pager->p_BeginList(String(L"NAVLINKS",8));
		for(int t_i2=0;t_i2<t_pLinkIdents.Length();t_i2=t_i2+1){
			m_pager->p_AddItem();
			m_pager->p_SetString(String(L"IDENT",5),t_pLinkIdents[t_i2]);
			m_pager->p_SetString(String(L"URL",3),t_pLinkUrls[t_i2]);
		}
		m_pager->p_EndList();
	}
	m_pager->p_SetString(String(L"CONTENT",7),t_pContents);
	return m_pager->p_MakePage();
}
void c_Makedocs::p__WritePage(String t_pFile,String t_pContents){
	if(!m_opt_printdocs && !m_opt_printtree){
		SaveString(t_pContents,String(L"docs/html/",10)+t_pFile);
	}else{
		if(m_opt_printdocs){
			bbPrint(String(L">>> OUTPUT FOR FILE: ",21)+t_pFile);
			bbPrint(t_pContents);
		}
	}
}
Array<String > c_Makedocs::p__LinkUrlsFromIdents(Array<String > t_pIdents){
	Array<String > t_urls=Array<String >(t_pIdents.Length());
	for(int t_i=0;t_i<t_pIdents.Length();t_i=t_i+1){
		String t_txt=String();
		for(int t_j=0;t_j<=t_i;t_j=t_j+1){
			if((t_txt).Length()!=0){
				t_txt=t_txt+String(L"_",1);
			}
			t_txt=t_txt+t_pIdents[t_j];
		}
		t_urls[t_i]=t_urls[t_i]+(t_txt+String(L".html",5));
	}
	return t_urls;
}
void c_Makedocs::p_WriteDocs(){
	m_template=LoadString(m_templatedir+String(L"/page_template.html",19));
	m_pager=(new c_PageMaker)->m_new(m_template);
	m_marker=(new c_Markdown)->m_new((this),(this));
	c_DocDeclStack* t_decls=m_rootdocs->p_GetChilds2(211,true);
	p_ClearErrInfo();
	m_curdecl=m_rootdocs;
	if((t_decls)!=0){
		int t_cnt=t_decls->p_Length2();
		m_iconlinks_url=Array<String >(t_cnt);
		m_iconlinks_icon=Array<String >(t_cnt);
		int t_i=0;
		c_Enumerator2* t_=t_decls->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			m_iconlinks_url[t_i]=p_BuildDocLink(t_d->m_parent,0);
			m_iconlinks_icon[t_i]=t_d->m_ident;
			t_i+=1;
		}
	}
	Array<String > t_linkidents=Array<String >();
	Array<String > t_linkurls=Array<String >();
	String t_page=String();
	String t_file=String();
	String t_content=String();
	String t_scopetemplate=LoadString(m_templatedir+String(L"/scope_template.html",20));
	c_PageMaker* t_scopepager=(new c_PageMaker)->m_new(t_scopetemplate);
	c_DocDeclStack* t_modules=m_rootmodules->p_GetChilds2(901,true);
	if((t_modules)!=0){
		c_Enumerator2* t_2=t_modules->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_DocDecl* t_m=t_2->p_NextObject();
			p_SetErrInfoFile(t_m->m_ident);
			m_curdecl=t_m;
			String t_3[]={String(L"Modules",7),t_m->p_GetIdent()};
			t_linkidents=Array<String >(t_3,2);
			String t_4[]={String(L"Modules.html",12),p_ResolveDocLink(t_m->m_ident)};
			t_linkurls=Array<String >(t_4,2);
			t_page=m_apidoccer->p_ApplyModuleTemplate(t_m,t_scopepager);
			t_content=p__ApplyPageTemplate(t_linkidents,t_linkurls,t_page);
			t_file=p_BuildDocLink(t_m,0);
			p__WritePage(t_file,t_content);
			if((t_m->m_childs)!=0){
				t_linkidents=t_linkidents.Resize(3);
				t_linkurls=t_linkurls.Resize(3);
				c_Enumerator2* t_5=t_m->m_childs->p_ObjectEnumerator();
				while(t_5->p_HasNext()){
					c_DocDecl* t_c=t_5->p_NextObject();
					if(t_c->m_kind==301 || t_c->m_kind==302){
						p_SetErrInfoFile(t_c->p_GetUniqueIdent());
						m_curdecl=t_c;
						t_linkidents[2]=t_c->m_ident;
						t_linkurls[2]=p_ResolveDocLink(t_c->p_GetUniqueIdent());
						t_page=m_apidoccer->p_ApplyClassTemplate(t_c,t_scopepager);
						t_content=p__ApplyPageTemplate(t_linkidents,t_linkurls,t_page);
						t_file=p_BuildDocLink(t_c,0);
						p__WritePage(t_file,t_content);
					}
				}
			}
		}
	}
	c_DocDeclStack* t_docs=m_rootdocs->p_GetChilds2(902,true);
	if((t_docs)!=0){
		c_Enumerator2* t_6=t_docs->p_ObjectEnumerator();
		while(t_6->p_HasNext()){
			c_DocDecl* t_d2=t_6->p_NextObject();
			p_SetErrInfoFile(t_d2->m_ident);
			m_curdecl=t_d2;
			if(t_d2->m_ident!=String(L"Home",4)){
				t_linkidents=t_d2->p_GetUniqueIdent().Split(String(L"/",1));
				t_linkurls=p__LinkUrlsFromIdents(t_linkidents);
			}else{
				t_linkidents=Array<String >();
				t_linkurls=Array<String >();
			}
			t_page=m_marker->p_ToHtml(t_d2->p_GetTextOfChild(805));
			t_content=p__ApplyPageTemplate(t_linkidents,t_linkurls,t_page);
			t_file=p_BuildDocLink(t_d2,0);
			p__WritePage(t_file,t_content);
		}
	}
	String t_indextemplate=LoadString(m_templatedir+String(L"/index_template.html",20));
	c_PageMaker* t_indexpager=(new c_PageMaker)->m_new(t_indextemplate);
	m_curdecl=m_rootdocs;
	String t_txt=String();
	c_NodeEnumerator2* t_7=m_indexes->p_ObjectEnumerator();
	while(t_7->p_HasNext()){
		c_Node4* t_index=t_7->p_NextObject();
		p_SetErrInfoFile(t_index->p_Key());
		t_linkidents=t_index->p_Key().Split(String(L"/",1));
		t_linkurls=p__LinkUrlsFromIdents(t_linkidents);
		String t_indexname=t_linkidents[t_linkidents.Length()-1];
		t_indexpager->p_Clear();
		t_indexpager->p_SetString(String(L"INDEX",5),t_indexname);
		t_indexpager->p_BeginList(String(L"ITEMS",5));
		if((t_index->p_Value())!=0){
			c_NodeEnumerator3* t_8=t_index->p_Value()->p_ObjectEnumerator();
			while(t_8->p_HasNext()){
				c_Node3* t_item=t_8->p_NextObject();
				c_DocDecl* t_decl=t_item->p_Value();
				t_indexpager->p_AddItem();
				m_curdecl=t_decl;
				m_apidoccer->p_SetPagerStrings(t_indexpager,t_decl,true);
			}
		}
		t_indexpager->p_EndList();
		t_page=t_indexpager->p_MakePage();
		t_content=p__ApplyPageTemplate(t_linkidents,t_linkurls,t_page);
		t_file=t_linkurls[t_linkurls.Length()-1];
		p__WritePage(t_file,t_content);
	}
}
void c_Makedocs::p_WriteExamples(){
	if(m_opt_printdocs || m_opt_printtree){
		return;
	}
	p__AssertPath(String(L"docs/html/examples",18));
	c_DocDeclStack* t_examples=m_rootmodules->p_GetChilds2(801,true);
	if((t_examples)!=0){
		c_Enumerator2* t_=t_examples->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_ex=t_->p_NextObject();
			String t_file=t_ex->p_GetTextOfChild(222);
			String t_source=t_ex->m_ident;
			int t_p0=t_source.Find(String(L"<pre>",5),0);
			int t_p1=t_source.Find(String(L"</pre>",6),t_p0+5);
			if(t_p0>=0 && t_p1>=0){
				t_source=t_source.Slice(t_p0+5,t_p1);
				SaveString(t_source,String(L"docs/html/examples/",19)+t_file);
			}
		}
	}
}
c_Makedocs* c_Makedocs::m_new(){
	bbPrint(String(L"Makedocs 2018-12-21",19));
	p_ParseAppArgs();
	p_BrowseToCxRoot();
	p_LoadTemplateName();
	p_LoadModulesPath();
	p_LoadDocPath();
	p_CleanUpEarly();
	p_CopyTemplateData();
	m_rootdecl=(new c_DocDecl)->m_new(100,String());
	m_rootmodules=(new c_DocDecl)->m_new(101,String());
	m_rootdocs=(new c_DocDecl)->m_new(102,String());
	m_rootdecl->p_Add(m_rootmodules);
	m_rootdecl->p_Add(m_rootdocs);
	m_branch3rdparty=(new c_DocDecl)->m_new(903,String(L"3rd Party Docs",14));
	m_branch3rdparty->p_Add((new c_DocDecl)->m_new(850,String(L"All 3rd Party Docs",18)));
	m_rootdocs->p_Add(m_branch3rdparty);
	bbPrint(String(L"parsing APIs...",15));
	m_apidoccer=(new c_ApiDoccer)->m_new(this);
	m_apidoccer->p_Parse();
	bbPrint(String(L"parsing cerberusdocs...",23));
	m_docdoccer=(new c_DocDoccer)->m_new(this);
	m_docdoccer->p_Doc();
	bbPrint(String(L"parsing 3rd party docs...",25));
	m_docdoccer->p_Doc3rdParty();
	bbPrint(String(L"completing decls...",19));
	p_Complete();
	p_CreateIndexes();
	m_rootdecl->p_Sort();
	bbPrint(String(L"writing decl files...",21));
	p_WriteDeclFiles();
	bbPrint(String(L"writing docs...",15));
	p_WriteDocs();
	bbPrint(String(L"writing examples...",19));
	p_WriteExamples();
	if(m_opt_printtree){
		bbPrint(m_rootdecl->p_ToString(0));
	}
	bbPrint(String(L"done!",5));
	return this;
}
String c_Makedocs::p_ResolveLink(String t_pLink,String t_pAltText){
	if(t_pLink.StartsWith(String(L"<img>",5))){
		String t_src=t_pLink.Slice(5);
		String t_html=String(L"<img src=\"",10)+t_src+String(L"\"",1);
		if((t_pAltText).Length()!=0){
			t_html=t_html+(String(L" alt=\"",6)+t_pAltText+String(L"\"",1));
		}
		t_html=t_html+String(L" />",3);
		return t_html;
	}
	String t_link=t_pLink;
	String t_alt=t_pAltText;
	if(!((t_pAltText).Length()!=0)){
		t_alt=t_pLink;
	}
	if(t_pLink.StartsWith(String(L"http://",7)) || t_pLink.StartsWith(String(L"https://",8)) || t_pLink.StartsWith(String(L"#",1)) || t_pLink.StartsWith(String(L"../",3)) || t_pLink.StartsWith(String(L"./",2))){
		String t_html2=String(L"<a href=\"",9)+t_link+String(L"\">",2)+t_alt+String(L"</a>",4);
		return t_html2;
	}
	if(!((t_pAltText).Length()!=0)){
		int t_p=t_alt.FindLast(String(L"/",1));
		if(t_p>=0){
			t_alt=t_alt.Slice(t_p+1);
		}
	}
	t_link=p_ResolveDocLink(t_link);
	if(m_resolvecode<0){
		return String(L"<a href=\"",9)+t_link+String(L"\" class=\"unresolved\">",21)+t_alt+String(L"</a>",4);
	}
	return String(L"<a href=\"",9)+t_link+String(L"\">",2)+t_alt+String(L"</a>",4);
}
String c_Makedocs::p_BeginPrettyBlock(){
	m_blocks=(new c_IntStack)->m_new2();
	return String(L"<div class=\"pretty\">",20);
}
String c_Makedocs::p_EndPrettyBlock(){
	return String(L"</div>",6);
}
String c_Makedocs::p_PrettifyLine(String t_pText){
	c_Toker* t_toker=(new c_Toker)->m_new(String(L"docs",4),t_pText);
	String t_css=String(L"d",1);
	String t__css=String();
	String t_txt=String();
	String t_html=String();
	do{
		t_toker->p_NextToke();
		t_txt=t_toker->m__toke;
		if(t_toker->m__tokeType==11 || t_toker->m__tokeType==0){
			break;
		}
		if(t_toker->m__toke==String(L"#",1)){
			t_toker->p_NextToke();
			if(t_toker->m__toke.ToLower()==String(L"rem",3)){
				t_css=String(L"r",1);
				m_blocks->p_Push7(1);
			}else{
				if(t_toker->m__toke.ToLower()==String(L"end",3)){
					if(m_blocks->p_Length2()>0){
						t_css=String(L"r",1);
						m_blocks->p_Pop();
					}else{
						t_css=String(L"d",1);
					}
				}else{
					t_css=String(L"d",1);
				}
			}
			t_txt=t_txt+t_toker->m__toke;
		}else{
			if(m_blocks->p_Length2()>0){
				t_css=String(L"r",1);
			}else{
				int t_11=t_toker->m__tokeType;
				if(t_11==9){
					t_css=String(L"r",1);
					if(t_txt.EndsWith(String(L"\n",1))){
						t_txt=t_txt.Slice(0,-1);
					}
					if(t_txt.EndsWith(String(L"\r\n",2))){
						t_txt=t_txt.Slice(0,-2);
					}
					if(t_txt.EndsWith(String(L"\r",1))){
						t_txt=t_txt.Slice(0,-1);
					}
				}else{
					if(t_11==3){
						t_css=String(L"k",1);
					}else{
						if(t_11==2){
							t_css=String(L"i",1);
						}else{
							if(t_11==4 || t_11==5 || t_11==6){
								t_css=String(L"l",1);
							}else{
								if(t_11==8){
									t_css=String(L"d",1);
								}
							}
						}
					}
				}
			}
		}
		if(t_css!=t__css){
			if((t__css).Length()!=0){
				t_html=t_html+String(L"</code>",7);
			}
			if((t_css).Length()!=0){
				t_html=t_html+(String(L"<code class=\"",13)+t_css+String(L"\">",2));
			}
		}
		t_html=t_html+bb_stringutil_HtmlEscape(t_txt);
		t__css=t_css;
	}while(!(false));
	if((t__css).Length()!=0){
		t_html=t_html+String(L"</code>",7);
	}
	t_html=t_html+String(L"<br/>",5);
	return t_html;
}
void c_Makedocs::mark(){
	Object::mark();
}
c_Stack::c_Stack(){
	m_data=Array<String >();
	m_length=0;
}
c_Stack* c_Stack::m_new(){
	return this;
}
c_Stack* c_Stack::m_new2(Array<String > t_data){
	this->m_data=t_data.Slice(0);
	this->m_length=t_data.Length();
	return this;
}
void c_Stack::p_Push(String t_value){
	if(m_length==m_data.Length()){
		m_data=m_data.Resize(m_length*2+10);
	}
	m_data[m_length]=t_value;
	m_length+=1;
}
void c_Stack::p_Push2(Array<String > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push(t_values[t_offset+t_i]);
	}
}
void c_Stack::p_Push3(Array<String > t_values,int t_offset){
	p_Push2(t_values,t_offset,t_values.Length()-t_offset);
}
bool c_Stack::p_Equals(String t_lhs,String t_rhs){
	return t_lhs==t_rhs;
}
bool c_Stack::p_Contains(String t_value){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		if(p_Equals(m_data[t_i],t_value)){
			return true;
		}
	}
	return false;
}
bool c_Stack::p_IsEmpty(){
	return m_length==0;
}
String c_Stack::p_Get(int t_index){
	return m_data[t_index];
}
String c_Stack::m_NIL;
String c_Stack::p_Pop(){
	m_length-=1;
	String t_v=m_data[m_length];
	m_data[m_length]=m_NIL;
	return t_v;
}
Array<String > c_Stack::p_ToArray(){
	Array<String > t_t=Array<String >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		t_t[t_i]=m_data[t_i];
	}
	return t_t;
}
void c_Stack::mark(){
	Object::mark();
}
c_StringStack::c_StringStack(){
}
c_StringStack* c_StringStack::m_new(Array<String > t_data){
	c_Stack::m_new2(t_data);
	return this;
}
c_StringStack* c_StringStack::m_new2(){
	c_Stack::m_new();
	return this;
}
String c_StringStack::p_Join(String t_separator){
	return t_separator.Join(p_ToArray());
}
bool c_StringStack::p_Equals(String t_lhs,String t_rhs){
	return t_lhs==t_rhs;
}
void c_StringStack::mark(){
	c_Stack::mark();
}
String bb_os_ExtractDir(String t_path){
	int t_i=t_path.FindLast(String(L"/",1));
	if(t_i==-1){
		t_i=t_path.FindLast(String(L"\\",1));
	}
	if(t_i!=-1){
		return t_path.Slice(0,t_i);
	}
	return String();
}
c_List::c_List(){
	m__head=((new c_HeadNode)->m_new());
}
c_List* c_List::m_new(){
	return this;
}
c_Node* c_List::p_AddLast(String t_data){
	return (new c_Node)->m_new(m__head,m__head->m__pred,t_data);
}
c_List* c_List::m_new2(Array<String > t_data){
	Array<String > t_=t_data;
	int t_2=0;
	while(t_2<t_.Length()){
		String t_t=t_[t_2];
		t_2=t_2+1;
		p_AddLast(t_t);
	}
	return this;
}
bool c_List::p_IsEmpty(){
	return m__head->m__succ==m__head;
}
String c_List::p_RemoveFirst(){
	String t_data=m__head->m__succ->m__data;
	m__head->m__succ->p_Remove();
	return t_data;
}
bool c_List::p_Equals(String t_lhs,String t_rhs){
	return t_lhs==t_rhs;
}
c_Node* c_List::p_Find(String t_value,c_Node* t_start){
	while(t_start!=m__head){
		if(p_Equals(t_value,t_start->m__data)){
			return t_start;
		}
		t_start=t_start->m__succ;
	}
	return 0;
}
c_Node* c_List::p_Find2(String t_value){
	return p_Find(t_value,m__head->m__succ);
}
void c_List::p_RemoveFirst2(String t_value){
	c_Node* t_node=p_Find2(t_value);
	if((t_node)!=0){
		t_node->p_Remove();
	}
}
int c_List::p_Count(){
	int t_n=0;
	c_Node* t_node=m__head->m__succ;
	while(t_node!=m__head){
		t_node=t_node->m__succ;
		t_n+=1;
	}
	return t_n;
}
c_Enumerator* c_List::p_ObjectEnumerator(){
	return (new c_Enumerator)->m_new(this);
}
Array<String > c_List::p_ToArray(){
	Array<String > t_arr=Array<String >(p_Count());
	int t_i=0;
	c_Enumerator* t_=this->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		String t_t=t_->p_NextObject();
		t_arr[t_i]=t_t;
		t_i+=1;
	}
	return t_arr;
}
void c_List::mark(){
	Object::mark();
}
c_StringList::c_StringList(){
}
c_StringList* c_StringList::m_new(Array<String > t_data){
	c_List::m_new2(t_data);
	return this;
}
c_StringList* c_StringList::m_new2(){
	c_List::m_new();
	return this;
}
bool c_StringList::p_Equals(String t_lhs,String t_rhs){
	return t_lhs==t_rhs;
}
void c_StringList::mark(){
	c_List::mark();
}
c_Node::c_Node(){
	m__succ=0;
	m__pred=0;
	m__data=String();
}
c_Node* c_Node::m_new(c_Node* t_succ,c_Node* t_pred,String t_data){
	m__succ=t_succ;
	m__pred=t_pred;
	m__succ->m__pred=this;
	m__pred->m__succ=this;
	m__data=t_data;
	return this;
}
c_Node* c_Node::m_new2(){
	return this;
}
int c_Node::p_Remove(){
	m__succ->m__pred=m__pred;
	m__pred->m__succ=m__succ;
	return 0;
}
void c_Node::mark(){
	Object::mark();
}
c_HeadNode::c_HeadNode(){
}
c_HeadNode* c_HeadNode::m_new(){
	c_Node::m_new2();
	m__succ=(this);
	m__pred=(this);
	return this;
}
void c_HeadNode::mark(){
	c_Node::mark();
}
c_Enumerator::c_Enumerator(){
	m__list=0;
	m__curr=0;
}
c_Enumerator* c_Enumerator::m_new(c_List* t_list){
	m__list=t_list;
	m__curr=t_list->m__head->m__succ;
	return this;
}
c_Enumerator* c_Enumerator::m_new2(){
	return this;
}
bool c_Enumerator::p_HasNext(){
	while(m__curr->m__succ->m__pred!=m__curr){
		m__curr=m__curr->m__succ;
	}
	return m__curr!=m__list->m__head;
}
String c_Enumerator::p_NextObject(){
	String t_data=m__curr->m__data;
	m__curr=m__curr->m__succ;
	return t_data;
}
void c_Enumerator::mark(){
	Object::mark();
}
Array<String > bb_os_LoadDir(String t_path,bool t_recursive,bool t_hidden){
	c_StringList* t_dirs=(new c_StringList)->m_new2();
	c_StringList* t_files=(new c_StringList)->m_new2();
	t_dirs->p_AddLast(String());
	while(!t_dirs->p_IsEmpty()){
		String t_dir=t_dirs->p_RemoveFirst();
		Array<String > t_=LoadDir(t_path+String(L"/",1)+t_dir);
		int t_2=0;
		while(t_2<t_.Length()){
			String t_f=t_[t_2];
			t_2=t_2+1;
			if(!t_hidden && t_f.StartsWith(String(L".",1))){
				continue;
			}
			if((t_dir).Length()!=0){
				t_f=t_dir+String(L"/",1)+t_f;
			}
			int t_1=FileType(t_path+String(L"/",1)+t_f);
			if(t_1==1){
				t_files->p_AddLast(t_f);
			}else{
				if(t_1==2){
					if(t_recursive){
						t_dirs->p_AddLast(t_f);
					}else{
						t_files->p_AddLast(t_f);
					}
				}
			}
		}
	}
	return t_files->p_ToArray();
}
int bb_os_DeleteDir(String t_path,bool t_recursive){
	if(!t_recursive){
		return DeleteDir(t_path);
	}
	int t_4=FileType(t_path);
	if(t_4==0){
		return 1;
	}else{
		if(t_4==1){
			return 0;
		}
	}
	Array<String > t_=LoadDir(t_path);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_f=t_[t_2];
		t_2=t_2+1;
		if(t_f==String(L".",1) || t_f==String(L"..",2)){
			continue;
		}
		String t_fpath=t_path+String(L"/",1)+t_f;
		if(FileType(t_fpath)==2){
			if(!((bb_os_DeleteDir(t_fpath,true))!=0)){
				return 0;
			}
		}else{
			if(!((DeleteFile(t_fpath))!=0)){
				return 0;
			}
		}
	}
	return DeleteDir(t_path);
}
int bb_os_CopyDir(String t_srcpath,String t_dstpath,bool t_recursive,bool t_hidden){
	if(FileType(t_srcpath)!=2){
		return 0;
	}
	Array<String > t_files=LoadDir(t_srcpath);
	int t_2=FileType(t_dstpath);
	if(t_2==0){
		if(!((CreateDir(t_dstpath))!=0)){
			return 0;
		}
	}else{
		if(t_2==1){
			return 0;
		}
	}
	Array<String > t_=t_files;
	int t_3=0;
	while(t_3<t_.Length()){
		String t_f=t_[t_3];
		t_3=t_3+1;
		if(!t_hidden && t_f.StartsWith(String(L".",1))){
			continue;
		}
		String t_srcp=t_srcpath+String(L"/",1)+t_f;
		String t_dstp=t_dstpath+String(L"/",1)+t_f;
		int t_32=FileType(t_srcp);
		if(t_32==1){
			if(!((CopyFile(t_srcp,t_dstp))!=0)){
				return 0;
			}
		}else{
			if(t_32==2){
				if(t_recursive && !((bb_os_CopyDir(t_srcp,t_dstp,t_recursive,t_hidden))!=0)){
					return 0;
				}
			}
		}
	}
	return 1;
}
c_DocDecl::c_DocDecl(){
	m_parent=0;
	m_kind=0;
	m_ident=String();
	m_uid=0;
	m_canbefound=false;
	m_childs=0;
	m__lastsid=0;
	m_target=0;
}
int c_DocDecl::m__uid;
bool c_DocDecl::p_Canbefound(){
	int t_1=m_kind;
	if(t_1==902 || t_1==901 || t_1==903 || t_1==301 || t_1==302 || t_1==310 || t_1==320 || t_1==321 || t_1==403 || t_1==405 || t_1==406 || t_1==410 || t_1==420 || t_1==421 || t_1==423 || t_1==601){
		return true;
	}
	return false;
}
c_DocDecl* c_DocDecl::m_new(int t_pKind,String t_pIdent){
	m_parent=0;
	m_kind=t_pKind;
	m_ident=t_pIdent;
	m_uid=m__uid;
	m__uid+=1;
	m_canbefound=p_Canbefound();
	return this;
}
c_DocDecl* c_DocDecl::m_new2(){
	return this;
}
void c_DocDecl::p_Add(c_DocDecl* t_pDecl){
	if(!((m_childs)!=0)){
		m_childs=(new c_DocDeclStack)->m_new();
	}
	m_childs->p_Push4(t_pDecl);
	t_pDecl->m_parent=this;
}
void c_DocDecl::p_Add2(c_Stack2* t_pDecls){
	c_Enumerator2* t_=t_pDecls->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_DocDecl* t_d=t_->p_NextObject();
		p_Add(t_d);
	}
}
c_DocDecl* c_DocDecl::p_GetChild(int t_pKind){
	if((m_childs)!=0){
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_c->m_kind==t_pKind){
				return t_c;
			}
		}
	}
	return 0;
}
c_DocDecl* c_DocDecl::p_GetChild2(String t_pIdent){
	if((m_childs)!=0){
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_c->m_ident==t_pIdent){
				return t_c;
			}
		}
	}
	return 0;
}
c_DocDecl* c_DocDecl::p_GetChild3(String t_pIdent,int t_pKind){
	if((m_childs)!=0){
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_c->m_ident==t_pIdent && (t_pKind==0 || t_c->m_kind==t_pKind)){
				return t_c;
			}
		}
	}
	return 0;
}
c_DocDeclStack* c_DocDecl::p_GetChilds(String t_pIdent,bool t_pRecursive){
	if((m_childs)!=0){
		c_DocDeclStack* t_ret=(new c_DocDeclStack)->m_new();
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_c->m_ident==t_pIdent){
				t_ret->p_Push4(t_c);
			}
			if(t_pRecursive){
				c_DocDeclStack* t_cret=t_c->p_GetChilds(t_pIdent,true);
				if((t_cret)!=0){
					t_ret->p_Push6(t_cret->p_ToArray(),0);
				}
			}
		}
		if((t_ret->p_Length2())!=0){
			return t_ret;
		}
	}
	return 0;
}
c_DocDeclStack* c_DocDecl::p_GetChilds2(int t_pKind,bool t_pRecursive){
	if((m_childs)!=0){
		c_DocDeclStack* t_ret=(new c_DocDeclStack)->m_new();
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_pKind==0 || t_c->m_kind==t_pKind){
				t_ret->p_Push4(t_c);
			}
			if(t_pRecursive){
				c_DocDeclStack* t_cret=t_c->p_GetChilds2(t_pKind,true);
				if((t_cret)!=0){
					t_ret->p_Push6(t_cret->p_ToArray(),0);
				}
			}
		}
		if((t_ret->p_Length2())!=0){
			return t_ret;
		}
	}
	return 0;
}
c_DocDecl* c_DocDecl::p_GetScope(int t_pKind){
	c_DocDecl* t_decl=this;
	while(t_decl->m_kind!=t_pKind){
		if((t_decl->m_parent)!=0){
			t_decl=t_decl->m_parent;
		}else{
			return 0;
		}
	}
	return t_decl;
}
c_DocDecl* c_DocDecl::m__root;
c_DocDecl* c_DocDecl::m__modroot;
c_DocDeclStack* c_DocDecl::m__modules;
c_DocDecl* c_DocDecl::m__docroot;
c_DocDeclStack* c_DocDecl::m__documents;
c_StringStack* c_DocDecl::m__primnames;
void c_DocDecl::p_PrepareFinder(){
	m__root=p_GetScope(100);
	m__modroot=m__root->p_GetChild(101);
	m__modules=m__modroot->p_GetChilds2(901,true);
	m__docroot=m__root->p_GetChild(102);
	m__documents=m__docroot->p_GetChilds2(902,true);
	c_DocDeclStack* t_alsodocuments=m__docroot->p_GetChilds2(903,true);
	if((t_alsodocuments)!=0){
		m__documents->p_Push6(t_alsodocuments->p_ToArray(),0);
	}
	String t_[]={String(L"Void",4),String(L"Bool",4),String(L"Int",3),String(L"Float",5),String(L"String",6)};
	m__primnames->p_Push3(Array<String >(t_,5),0);
}
String c_DocDecl::p_GetType(bool t_pPutInLink,bool t_pMarkOptionals){
	String t_str=String();
	c_DocDecl* t_t=p_GetChild(500);
	if((t_t)!=0){
		t_str=t_t->m_ident;
		if(t_pPutInLink){
			c_DocDecl* t_gparam=0;
			c_DocDecl* t_scope=p_GetScope(301);
			if((t_scope)!=0){
				t_gparam=t_scope->p_GetChild3(t_str,400);
			}
			if(!((t_gparam)!=0)){
				t_str=String(L"[[",2)+t_str+String(L"]]",2);
			}
		}
		t_str=t_str+t_t->p_GetType(t_pPutInLink,false);
	}
	c_DocDeclStack* t_args=p_GetChilds2(551,false);
	if((t_args)!=0){
		String t_astr=String();
		c_Enumerator2* t_=t_args->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_a=t_->p_NextObject();
			if((t_astr).Length()!=0){
				t_astr=t_astr+String(L",",1);
			}
			t_astr=t_astr+(t_a->m_ident+t_a->p_GetType(t_pPutInLink,false));
		}
		if(t_pPutInLink){
			t_str=t_str+(String(L"&lt;",4)+t_astr+String(L"&gt;",4));
		}else{
			t_str=t_str+(String(L"<",1)+t_astr+String(L">",1));
		}
	}
	c_DocDecl* t_arr=p_GetChild(550);
	while((t_arr)!=0){
		t_str=t_str+String(L"[]",2);
		t_arr=t_arr->p_GetChild(550);
	}
	if(t_pMarkOptionals && ((t_t)!=0) && ((t_t->m_parent->p_GetChild(502))!=0)){
		t_str=String(L"[",1)+t_str+String(L"]",1);
	}
	return t_str;
}
String c_DocDecl::p_GetUniqueIdent(){
	String t__scope=String();
	String t__params=String();
	String t_str=String();
	if((m_childs)!=0){
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_c->m_kind==201){
				t__scope=t_c->m_ident;
				if((t__params).Length()!=0){
					break;
				}
			}else{
				if(t_c->m_kind==202){
					t__params=t_c->m_ident;
					if((t__scope).Length()!=0){
						break;
					}
				}
			}
		}
	}
	return t__scope+m_ident+t__params;
}
c_StringMap2* c_DocDecl::m__primlinks;
int c_DocDecl::m__searchid;
c_DocDecl* c_DocDecl::p__FindInHere(Array<String > t_pIdentPath,int t_pLevel,String t_pParams,int t_pKind){
	if(t_pLevel==0){
		if(m__lastsid==m__searchid){
			return 0;
		}
		m__lastsid=m__searchid;
	}
	bool t_islastlevel=t_pLevel==t_pIdentPath.Length()-1;
	String t__ident=t_pIdentPath[t_pLevel];
	if((m_childs)!=0){
		bool t_cbf=false;
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			t_cbf=t_c->m_canbefound;
			if(t_c->m_kind==900 && !t_islastlevel){
				t_cbf=true;
			}
			if(t_cbf){
				String t_cident=t_c->m_ident;
				if(t_c->m_kind==901 || t_c->m_kind==900){
					int t_p0=t_cident.FindLast(String(L".",1));
					if(t_p0>=0){
						t_cident=t_cident.Slice(t_p0+1);
					}
				}
				if(t_islastlevel){
					if(t_cident==t__ident && (t_pKind==0 || t_c->m_kind==t_pKind)){
						if((t_pParams).Length()!=0){
							c_DocDecl* t_p=t_c->p_GetChild(202);
							if(!((t_p)!=0)){
								continue;
							}
							if(t_p->m_ident!=t_pParams){
								continue;
							}
						}
						return t_c;
					}
				}else{
					if(t_cident==t__ident){
						return t_c->p__FindInHere(t_pIdentPath,t_pLevel+1,t_pParams,t_pKind);
					}
				}
			}
		}
	}
	return 0;
}
c_DocDecl* c_DocDecl::p__FoundDecl(c_DocDecl* t_pDecl,bool t_pAddToPrimLinks){
	if(t_pAddToPrimLinks){
		m__primlinks->p_Add3(t_pDecl->m_ident,t_pDecl);
	}
	return t_pDecl;
}
c_DocDecl* c_DocDecl::p_FindFromHere(String t_pIdent,int t_pKind){
	bool t_addtoprimlinks=false;
	if(m__primnames->p_Contains(t_pIdent)){
		c_DocDecl* t_decl=m__primlinks->p_Get2(t_pIdent);
		if((t_decl)!=0){
			return t_decl;
		}
		t_addtoprimlinks=true;
	}
	m__searchid+=1;
	c_DocDecl* t_scope=0;
	c_DocDecl* t_decl2=0;
	c_DocDeclStack* t_decls=0;
	Array<String > t_idents=Array<String >();
	String t_params=String();
	String t_docpathtomatch=String();
	if(t_pIdent.Contains(String(L"/",1))){
		t_idents=t_pIdent.Split(String(L"/",1));
		if(t_idents[0]==String()){
			t_idents=t_idents.Slice(1);
			t_scope=m__docroot;
		}else{
			t_scope=this;
		}
	}else{
		String t__ident=String();
		if(t_pIdent.Contains(String(L"(",1))){
			int t_p0=t_pIdent.Find(String(L"(",1),0);
			t_params=t_pIdent.Slice(t_p0);
			t__ident=t_pIdent.Slice(0,t_p0);
		}else{
			t__ident=t_pIdent;
		}
		t_idents=t__ident.Split(String(L".",1));
		if(t_idents[0]==String()){
			t_idents=t_idents.Slice(1);
			t_scope=m__modroot;
		}else{
			t_scope=p_GetScope(301);
			if(!((t_scope)!=0)){
				t_scope=p_GetScope(302);
			}
			if(!((t_scope)!=0)){
				t_scope=p_GetScope(901);
			}
			if(!((t_scope)!=0)){
				t_scope=m__modroot;
			}
		}
	}
	do{
		int t_4=t_scope->m_kind;
		if(t_4==301 || t_4==302){
			t_decl2=t_scope->p__FindInHere(t_idents,0,t_params,t_pKind);
			if((t_decl2)!=0){
				return p__FoundDecl(t_decl2,false);
			}
			t_decls=t_scope->p_GetChilds2(401,false);
			if((t_decls)!=0){
				c_Enumerator2* t_=t_decls->p_ObjectEnumerator();
				while(t_->p_HasNext()){
					c_DocDecl* t_d=t_->p_NextObject();
					if((t_d->m_target)!=0){
						t_decl2=t_d->m_target->p__FindInHere(t_idents,0,t_params,t_pKind);
						if((t_decl2)!=0){
							return p__FoundDecl(t_decl2,false);
						}
					}
				}
			}
			t_decls=t_scope->p_GetChilds2(402,false);
			if((t_decls)!=0){
				c_Enumerator2* t_2=t_decls->p_ObjectEnumerator();
				while(t_2->p_HasNext()){
					c_DocDecl* t_d2=t_2->p_NextObject();
					if((t_d2->m_target)!=0){
						t_decl2=t_d2->m_target->p__FindInHere(t_idents,0,t_params,t_pKind);
						if((t_decl2)!=0){
							return p__FoundDecl(t_decl2,false);
						}
					}
				}
			}
			t_scope=t_scope->p_GetScope(901);
		}else{
			if(t_4==901 || t_4==900){
				t_decl2=t_scope->p__FindInHere(t_idents,0,t_params,t_pKind);
				if((t_decl2)!=0){
					return p__FoundDecl(t_decl2,false);
				}
				t_decls=t_scope->p_GetChilds2(300,false);
				if((t_decls)!=0){
					c_Enumerator2* t_3=t_decls->p_ObjectEnumerator();
					while(t_3->p_HasNext()){
						c_DocDecl* t_d3=t_3->p_NextObject();
						if((t_d3->m_target)!=0){
							t_decl2=t_d3->m_target->p__FindInHere(t_idents,0,t_params,t_pKind);
							if((t_decl2)!=0){
								return p__FoundDecl(t_decl2,false);
							}
						}
					}
				}
				t_scope=t_scope->m_parent;
			}else{
				if(t_4==101){
					t_decl2=t_scope->p__FindInHere(t_idents,0,t_params,t_pKind);
					if((t_decl2)!=0){
						return p__FoundDecl(t_decl2,false);
					}
					t_decls=m__modules;
					if((t_decls)!=0){
						c_Enumerator2* t_5=t_decls->p_ObjectEnumerator();
						while(t_5->p_HasNext()){
							c_DocDecl* t_d4=t_5->p_NextObject();
							t_decl2=t_d4->p__FindInHere(t_idents,0,t_params,t_pKind);
							if((t_decl2)!=0){
								return p__FoundDecl(t_decl2,false);
							}
						}
					}
					t_scope=m__docroot;
				}else{
					if(t_4==102){
						t_decl2=t_scope->p__FindInHere(t_idents,0,t_params,t_pKind);
						if((t_decl2)!=0){
							return p__FoundDecl(t_decl2,t_addtoprimlinks);
						}
						t_decls=m__documents;
						if((t_decls)!=0){
							c_Enumerator2* t_6=t_decls->p_ObjectEnumerator();
							while(t_6->p_HasNext()){
								c_DocDecl* t_d5=t_6->p_NextObject();
								t_decl2=t_d5->p__FindInHere(t_idents,0,t_params,t_pKind);
								if((t_decl2)!=0){
									return p__FoundDecl(t_decl2,t_addtoprimlinks);
								}
							}
						}
						return 0;
					}else{
						if(t_4==902 || t_4==903){
							t_decl2=t_scope->p__FindInHere(t_idents,0,t_params,t_pKind);
							if((t_decl2)!=0){
								return p__FoundDecl(t_decl2,false);
							}
							t_scope=t_scope->m_parent;
						}else{
							t_scope=m__docroot;
						}
					}
				}
			}
		}
	}while(!(false));
}
String c_DocDecl::p_GetIdent(){
	return m_ident;
}
String c_DocDecl::p_GetIdentWithParams(){
	if((m_childs)!=0){
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_c->m_kind==202){
				return p_GetIdent()+t_c->m_ident;
			}
		}
	}
	return p_GetIdent();
}
String c_DocDecl::p_GetTextOfChild(int t_pKind){
	c_DocDecl* t_d=p_GetChild(t_pKind);
	if((t_d)!=0){
		return t_d->m_ident;
	}
	return String();
}
void c_DocDecl::p_Sort(){
	if((m_childs)!=0){
		m_childs->p_Sort2(true);
	}
}
String c_DocDecl::p_GetKindName(){
	int t_2=m_kind;
	if(t_2==901){
		return String(L"Module",6);
	}else{
		if(t_2==301){
			return String(L"Class",5);
		}else{
			if(t_2==302){
				return String(L"Interface",9);
			}else{
				if(t_2==310){
					return String(L"Function",8);
				}else{
					if(t_2==320){
						return String(L"Const",5);
					}else{
						if(t_2==321){
							return String(L"Global",6);
						}else{
							if(t_2==403){
								return String(L"Method",6);
							}else{
								if(t_2==405){
									return String(L"Property",8);
								}else{
									if(t_2==406){
										return String(L"Method",6);
									}else{
										if(t_2==410){
											return String(L"Function",8);
										}else{
											if(t_2==420){
												return String(L"Const",5);
											}else{
												if(t_2==421){
													return String(L"Global",6);
												}else{
													if(t_2==423){
														return String(L"Field",5);
													}else{
														if(t_2==453){
															return String(L"Inherited_method",16);
														}else{
															if(t_2==455){
																return String(L"Inherited_property",18);
															}else{
																if(t_2==456){
																	return String(L"Inherited_method",16);
																}else{
																	if(t_2==460){
																		return String(L"Inherited_function",18);
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return String(L"Unspecified",11);
}
String c_DocDecl::p_GetScopeIdent(){
	return p_GetTextOfChild(201);
}
String c_DocDecl::p_GetDocType(bool t_pPutInLink){
	String t_str=p_GetType(t_pPutInLink,false);
	if((t_str).Length()!=0){
		t_str=String(L" : ",3)+t_str;
	}
	c_DocDecl* t_decl=p_GetChild(502);
	if((t_decl)!=0){
		if(!((t_str).Length()!=0) && m_kind!=601){
			t_str=String(L" := ",4)+t_decl->m_ident;
		}else{
			t_str=t_str+(String(L" = ",3)+t_decl->m_ident);
		}
	}
	c_DocDeclStack* t_decls=p_GetChilds2(600,false);
	if((t_decls)!=0){
		t_str=t_str+String(L" ( ",3);
		bool t_isfirst=true;
		c_Enumerator2* t_=t_decls->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			if(!t_isfirst){
				t_str=t_str+String(L", ",2);
			}
			t_str=t_str+(t_d->m_ident+String(L":",1));
			t_str=t_str+t_d->p_GetType(t_pPutInLink,false);
			c_DocDecl* t_decl2=t_d->p_GetChild(502);
			if((t_decl2)!=0){
				t_str=t_str+(String(L"=",1)+t_decl2->m_ident);
			}
			t_isfirst=false;
		}
		t_str=t_str+String(L" )",2);
	}else{
		int t_3=m_kind;
		if(t_3==310 || t_3==403 || t_3==405 || t_3==406 || t_3==410 || t_3==453 || t_3==455 || t_3==456 || t_3==460){
			t_str=t_str+String(L" ()",3);
		}
	}
	return t_str;
}
c_DocDecl* c_DocDecl::p__FindChild(String t_pIdent,String t_pParams,int t_pKind){
	if((m_childs)!=0){
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_c=t_->p_NextObject();
			if(t_c->m_canbefound){
				if(t_c->m_ident==t_pIdent && (t_pKind==0 || t_c->m_kind==t_pKind)){
					if((t_pParams).Length()!=0){
						c_DocDecl* t_p=t_c->p_GetChild(202);
						if(((t_p)!=0) && t_p->m_ident==t_pParams){
							return t_c;
						}
					}else{
						return t_c;
					}
				}
				if((t_c->m_childs)!=0){
					c_DocDecl* t_decl=t_c->p__FindChild(t_pIdent,t_pParams,t_pKind);
					if((t_decl)!=0){
						return t_decl;
					}
				}
			}else{
				if(t_c->m_kind==102 || t_c->m_kind==101){
					return t_c->p__FindChild(t_pIdent,t_pParams,t_pKind);
				}
			}
		}
	}
	return 0;
}
c_DocDecl* c_DocDecl::p_FindChild(String t_pIdent,int t_pKind){
	String t__ident=String();
	String t_params=String();
	if(t_pIdent.Contains(String(L"(",1))){
		int t_p0=t_pIdent.Find(String(L"(",1),0);
		t_params=t_pIdent.Slice(t_p0);
		t__ident=t_pIdent.Slice(0,t_p0);
	}else{
		t__ident=t_pIdent;
	}
	return p__FindChild(t__ident,t_params,t_pKind);
}
String c_DocDecl::p_GetGenType(){
	c_DocDeclStack* t_types=p_GetChilds2(400,false);
	if((t_types)!=0){
		String t_str=String();
		c_Enumerator2* t_=t_types->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_t=t_->p_NextObject();
			if((t_str).Length()!=0){
				t_str=t_str+String(L",",1);
			}
			t_str=t_str+t_t->m_ident;
		}
		return String(L"<",1)+t_str+String(L">",1);
	}
	return String();
}
String c_DocDecl::p_GetDocXType(){
	return p_GetDocType(true);
}
String c_DocDecl::p_GetTargetIdent(){
	if((m_target)!=0){
		return m_target->m_ident;
	}
	return m_ident;
}
String c_DocDecl::p_ToString(int t_pIndent){
	String t_indstr=String();
	for(int t_i=0;t_i<t_pIndent;t_i=t_i+1){
		t_indstr=t_indstr+String(L"  ",2);
	}
	String t_uidstr=(String(L"00000",5)+String(m_uid)).Slice(-6);
	t_indstr=t_indstr+String(m_kind)+String(L".",1)+t_uidstr+String(L" ",1);
	t_indstr=t_indstr+m_ident.Replace(String(L"\n",1),String(L"~n",2)).Replace(String(L"\r",1),String());
	if((m_target)!=0){
		String t_uidstr2=(String(L"00000",5)+String(m_target->m_uid)).Slice(-6);
		t_indstr=t_indstr+(String(L" (Targeting ",12)+String(m_target->m_kind)+String(L".",1)+t_uidstr2+String(L")",1));
	}
	t_indstr=t_indstr+String(L"\n",1);
	if((m_childs)!=0){
		c_Enumerator2* t_=m_childs->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			t_indstr=t_indstr+t_d->p_ToString(t_pIndent+1);
		}
	}
	return t_indstr;
}
void c_DocDecl::mark(){
	Object::mark();
}
c_Stack2::c_Stack2(){
	m_data=Array<c_DocDecl* >();
	m_length=0;
}
c_Stack2* c_Stack2::m_new(){
	return this;
}
c_Stack2* c_Stack2::m_new2(Array<c_DocDecl* > t_data){
	this->m_data=t_data.Slice(0);
	this->m_length=t_data.Length();
	return this;
}
void c_Stack2::p_Push4(c_DocDecl* t_value){
	if(m_length==m_data.Length()){
		m_data=m_data.Resize(m_length*2+10);
	}
	m_data[m_length]=t_value;
	m_length+=1;
}
void c_Stack2::p_Push5(Array<c_DocDecl* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push4(t_values[t_offset+t_i]);
	}
}
void c_Stack2::p_Push6(Array<c_DocDecl* > t_values,int t_offset){
	p_Push5(t_values,t_offset,t_values.Length()-t_offset);
}
c_Enumerator2* c_Stack2::p_ObjectEnumerator(){
	return (new c_Enumerator2)->m_new(this);
}
c_DocDecl* c_Stack2::m_NIL;
void c_Stack2::p_Length(int t_newlength){
	if(t_newlength<m_length){
		for(int t_i=t_newlength;t_i<m_length;t_i=t_i+1){
			m_data[t_i]=m_NIL;
		}
	}else{
		if(t_newlength>m_data.Length()){
			m_data=m_data.Resize(bb_math_Max(m_length*2+10,t_newlength));
		}
	}
	m_length=t_newlength;
}
int c_Stack2::p_Length2(){
	return m_length;
}
Array<c_DocDecl* > c_Stack2::p_ToArray(){
	Array<c_DocDecl* > t_t=Array<c_DocDecl* >(m_length);
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		t_t[t_i]=m_data[t_i];
	}
	return t_t;
}
bool c_Stack2::p_Equals2(c_DocDecl* t_lhs,c_DocDecl* t_rhs){
	return t_lhs==t_rhs;
}
void c_Stack2::p_RemoveEach(c_DocDecl* t_value){
	int t_i=0;
	int t_j=m_length;
	while(t_i<m_length){
		if(!p_Equals2(m_data[t_i],t_value)){
			t_i+=1;
			continue;
		}
		int t_b=t_i;
		int t_e=t_i+1;
		while(t_e<m_length && p_Equals2(m_data[t_e],t_value)){
			t_e+=1;
		}
		while(t_e<m_length){
			m_data[t_b]=m_data[t_e];
			t_b+=1;
			t_e+=1;
		}
		m_length-=t_e-t_b;
		t_i+=1;
	}
	t_i=m_length;
	while(t_i<t_j){
		m_data[t_i]=m_NIL;
		t_i+=1;
	}
}
c_DocDecl* c_Stack2::p_Get(int t_index){
	return m_data[t_index];
}
int c_Stack2::p_Compare(c_DocDecl* t_lhs,c_DocDecl* t_rhs){
	bbError(String(L"Unable to compare items",23));
	return 0;
}
bool c_Stack2::p__Less(int t_x,int t_y,int t_ascending){
	return p_Compare(m_data[t_x],m_data[t_y])*t_ascending<0;
}
void c_Stack2::p__Swap(int t_x,int t_y){
	c_DocDecl* t_t=m_data[t_x];
	m_data[t_x]=m_data[t_y];
	m_data[t_y]=t_t;
}
bool c_Stack2::p__Less2(int t_x,c_DocDecl* t_y,int t_ascending){
	return p_Compare(m_data[t_x],t_y)*t_ascending<0;
}
bool c_Stack2::p__Less3(c_DocDecl* t_x,int t_y,int t_ascending){
	return p_Compare(t_x,m_data[t_y])*t_ascending<0;
}
void c_Stack2::p__Sort(int t_lo,int t_hi,int t_ascending){
	if(t_hi<=t_lo){
		return;
	}
	if(t_lo+1==t_hi){
		if(p__Less(t_hi,t_lo,t_ascending)){
			p__Swap(t_hi,t_lo);
		}
		return;
	}
	int t_i=(t_hi-t_lo)/2+t_lo;
	if(p__Less(t_i,t_lo,t_ascending)){
		p__Swap(t_i,t_lo);
	}
	if(p__Less(t_hi,t_i,t_ascending)){
		p__Swap(t_hi,t_i);
		if(p__Less(t_i,t_lo,t_ascending)){
			p__Swap(t_i,t_lo);
		}
	}
	int t_x=t_lo+1;
	int t_y=t_hi-1;
	do{
		c_DocDecl* t_p=m_data[t_i];
		while(p__Less2(t_x,t_p,t_ascending)){
			t_x+=1;
		}
		while(p__Less3(t_p,t_y,t_ascending)){
			t_y-=1;
		}
		if(t_x>t_y){
			break;
		}
		if(t_x<t_y){
			p__Swap(t_x,t_y);
			if(t_i==t_x){
				t_i=t_y;
			}else{
				if(t_i==t_y){
					t_i=t_x;
				}
			}
		}
		t_x+=1;
		t_y-=1;
	}while(!(t_x>t_y));
	p__Sort(t_lo,t_y,t_ascending);
	p__Sort(t_x,t_hi,t_ascending);
}
void c_Stack2::p_Sort2(bool t_ascending){
	if(!((m_length)!=0)){
		return;
	}
	int t_t=1;
	if(!t_ascending){
		t_t=-1;
	}
	p__Sort(0,m_length-1,t_t);
}
void c_Stack2::mark(){
	Object::mark();
}
c_DocDeclStack::c_DocDeclStack(){
}
c_DocDeclStack* c_DocDeclStack::m_new(){
	c_Stack2::m_new();
	return this;
}
void c_DocDeclStack::p_Sort2(bool t_pAsc){
	c_Stack2::p_Sort2(t_pAsc);
	c_Enumerator2* t_=this->p_ObjectEnumerator();
	while(t_->p_HasNext()){
		c_DocDecl* t_d=t_->p_NextObject();
		t_d->p_Sort();
	}
}
int c_DocDeclStack::p_Compare(c_DocDecl* t_pLhs,c_DocDecl* t_pRhs){
	if(t_pLhs->m_kind<900 || t_pRhs->m_kind<900){
		if(t_pLhs->m_kind<t_pRhs->m_kind){
			return -1;
		}
		if(t_pLhs->m_kind>t_pRhs->m_kind){
			return 1;
		}
	}
	int t_5=t_pLhs->m_kind;
	if(t_5==400 || t_5==551 || t_5==600 || t_5==800 || t_5==801 || t_5==803 || t_5==804 || t_5==322 || t_5==422 || t_5==601){
		return t_pLhs->m_uid-t_pRhs->m_uid;
	}
	c_DocDecl* t_lhs=t_pLhs;
	c_DocDecl* t_rhs=t_pRhs;
	if((t_lhs->m_target)!=0){
		t_lhs=t_lhs->m_target;
	}
	if((t_rhs->m_target)!=0){
		t_rhs=t_rhs->m_target;
	}
	int t_c=t_lhs->m_ident.ToLower().Compare(t_rhs->m_ident.ToLower());
	if(t_c!=0){
		return t_c;
	}
	String t_lident=t_lhs->p_GetIdentWithParams();
	String t_rident=t_rhs->p_GetIdentWithParams();
	return t_lident.Compare(t_rident);
}
void c_DocDeclStack::mark(){
	c_Stack2::mark();
}
c_Enumerator2::c_Enumerator2(){
	m_stack=0;
	m_index=0;
}
c_Enumerator2* c_Enumerator2::m_new(c_Stack2* t_stack){
	this->m_stack=t_stack;
	return this;
}
c_Enumerator2* c_Enumerator2::m_new2(){
	return this;
}
bool c_Enumerator2::p_HasNext(){
	return m_index<m_stack->p_Length2();
}
c_DocDecl* c_Enumerator2::p_NextObject(){
	m_index+=1;
	return m_stack->m_data[m_index-1];
}
void c_Enumerator2::mark(){
	Object::mark();
}
int bb_math_Max(int t_x,int t_y){
	if(t_x>t_y){
		return t_x;
	}
	return t_y;
}
Float bb_math_Max2(Float t_x,Float t_y){
	if(t_x>t_y){
		return t_x;
	}
	return t_y;
}
c_ApiDoccer::c_ApiDoccer(){
	m_maker=0;
	m_cursect=0;
	m_curparagraph=0;
	m_parser=0;
}
c_ApiDoccer* c_ApiDoccer::m_new(c_Makedocs* t_pMakedocs){
	m_maker=t_pMakedocs;
	return this;
}
c_ApiDoccer* c_ApiDoccer::m_new2(){
	return this;
}
void c_ApiDoccer::p_LoadExamples(String t_pDirectory,c_DocDecl* t_pScope){
	Array<String > t_=LoadDir(t_pDirectory);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_file=t_[t_2];
		t_2=t_2+1;
		String t_path=t_pDirectory+String(L"/",1)+t_file;
		String t_ext=bb_os_ExtractExt(t_file);
		String t_name=bb_os_StripExt(t_file);
		if((t_ext==String(L"cxs",3) || t_ext==String(L"monkey",6)) && t_name.EndsWith(String(L"_example",8))){
			String t_source=LoadString(t_path);
			c_DocDecl* t_decl=(new c_DocDecl)->m_new(220,t_name.Slice(0,-8));
			t_decl->p_Add((new c_DocDecl)->m_new(221,t_source));
			t_pScope->p_Add(t_decl);
		}
	}
}
void c_ApiDoccer::p_AppendDocContents(c_DocDecl* t_pDecl,String t_pString){
	if(t_pString.StartsWith(String(L"Example:",8))){
		m_cursect=801;
		m_curparagraph=0;
		t_pString=t_pString.Slice(8);
	}else{
		if(t_pString.StartsWith(String(L"Links:",6))){
			m_cursect=802;
			m_curparagraph=0;
			t_pString=t_pString.Slice(6);
		}else{
			if(t_pString.StartsWith(String(L"Parameters:",11))){
				m_cursect=803;
				m_curparagraph=0;
				t_pString=t_pString.Slice(11);
			}else{
				if(t_pString.StartsWith(String(L"Returns:",8))){
					m_cursect=804;
					m_curparagraph=0;
					t_pString=t_pString.Slice(8);
				}
			}
		}
	}
	if(((t_pString.Trim()).Length()!=0) && !((m_curparagraph)!=0)){
		m_curparagraph=(new c_DocDecl)->m_new(m_cursect,String());
		t_pDecl->p_Add(m_curparagraph);
	}
	if((m_curparagraph)!=0){
		m_curparagraph->m_ident=m_curparagraph->m_ident+bb_stringutil_UnifyLineEndings(t_pString);
	}
}
void c_ApiDoccer::p_Error(String t_pMessage){
	m_maker->p_SetErrInfoLine(m_parser->p_GetCarretLine());
	m_maker->p_SetErrInfoChar(m_parser->p_GetCarretChar());
	m_maker->p_Error(t_pMessage);
	m_maker->p_ClearErrInfo();
}
void c_ApiDoccer::p_ParseDocFile(String t_pPath,c_DocDecl* t_pScope,String t_pModulePath){
	m_maker->p_SetErrInfoFile(t_pPath);
	int t_p0=t_pModulePath.FindLast(String(L".",1));
	String t_path=String();
	String t_ident=String();
	if((t_p0)!=0){
		t_path=t_pModulePath.Slice(0,t_p0+1);
		t_ident=t_pModulePath.Slice(t_p0+1);
	}else{
		t_ident=t_pModulePath;
	}
	c_DocDecl* t_mdecl=(new c_DocDecl)->m_new(901,t_pModulePath);
	c_DocDecl* t_curdecl=t_mdecl;
	m_cursect=800;
	m_curparagraph=0;
	try{
		String t_source=LoadString(t_pPath);
		m_parser=(new c_Parser)->m_new(t_source);
		m_parser->m__tokePos=0;
		while(!(m_parser->m__tokeType==0)){
			c_Toker* t_state=m_parser->p_Store();
			m_parser->p_NextSpace();
			if((m_parser->p_NextCdata(String(L"# ",2))).Length()!=0){
				m_parser->p_NextToke();
				t_curdecl=m_parser->p_ParseDecl(t_curdecl);
				m_parser->p_NextRestOfLine();
				m_cursect=800;
				m_curparagraph=0;
			}else{
				if((m_parser->p_NextCdata(String(L"'# ",3))).Length()!=0){
					m_parser->p_NextRestOfLine();
				}else{
					m_parser->p_Restore(t_state);
					String t_str=m_parser->p_NextRestOfLine();
					p_AppendDocContents(t_curdecl,t_str);
				}
			}
		}
		if((t_mdecl->p_GetChild(200))!=0){
			t_mdecl->m_canbefound=true;
			t_pScope->p_Add(t_mdecl);
		}else{
			m_maker->p_SetErrInfoLine(0);
			m_maker->p_SetErrInfoChar(0);
			m_maker->p_Error(String(L"Module header not found",23));
		}
	}catch(c_ThrowableString* t_message){
		p_Error(t_message->p_ToString2());
	}
}
void c_ApiDoccer::p_CopyDocsData(String t_pPath,String t_pModulePath){
	String t_path=bb_os_StripExt(t_pPath)+String(L".data",5);
	if(FileType(t_path)==2){
		String t_dst=String(L"docs/html/data/",15)+t_pModulePath.Replace(String(L".",1),String(L"/",1));
		m_maker->p_CopyDir(t_path,t_dst,true);
	}
}
void c_ApiDoccer::p_ParseSourceFile(String t_pPath,c_DocDecl* t_pScope,String t_pModulePath){
	m_maker->p_SetErrInfoFile(t_pPath);
	int t_p0=t_pModulePath.FindLast(String(L".",1));
	String t_path=String();
	String t_ident=String();
	if((t_p0)!=0){
		t_path=t_pModulePath.Slice(0,t_p0+1);
		t_ident=t_pModulePath.Slice(t_p0+1);
	}else{
		t_ident=t_pModulePath;
	}
	c_DocDecl* t_mdecl=0;
	c_DocDecl* t_doccdecl=0;
	c_DocDecl* t_curdecl=0;
	m_cursect=800;
	m_curparagraph=0;
	try{
		int t_docswitch=0;
		c_IntStack* t_docblocks=(new c_IntStack)->m_new2();
		bool t_inrem=false;
		bool t_indoc=false;
		String t_source=LoadString(t_pPath);
		m_parser=(new c_Parser)->m_new(t_source);
		m_parser->m__tokePos=0;
		while(!(m_parser->m__tokeType==0)){
			t_inrem=false;
			t_indoc=false;
			c_BackwardsEnumerator* t_=t_docblocks->p_Backwards()->p_ObjectEnumerator();
			while(t_->p_HasNext()){
				int t_b=t_->p_NextObject();
				if(t_b==0){
					t_indoc=true;
					break;
				}else{
					if(t_b==1){
						t_inrem=true;
						break;
					}
				}
			}
			c_Toker* t_state=m_parser->p_Store();
			m_parser->p_NextSpace();
			if((m_parser->p_NextCdata(String(L"#Rem",4))).Length()!=0){
				m_parser->p_NextToke();
				bool t_doanalyse=!(t_indoc || t_inrem);
				if(t_doanalyse && m_parser->p_PopToken(String(L"cerberusdoc",11),false)){
					t_docblocks->p_Push7(0);
					m_cursect=800;
					m_curparagraph=0;
					if(m_parser->p_PopUntilKeyword(String(L"module",6),false)){
						t_mdecl=(new c_DocDecl)->m_new(901,t_pModulePath);
						if((t_path).Length()!=0){
						}
						m_parser->p_ParseDecl(t_mdecl);
						t_docswitch=1;
						t_doccdecl=t_mdecl;
						t_curdecl=t_mdecl;
					}else{
						if(m_parser->p_PopToken(String(L"off",3),false)){
							t_docswitch=0;
							t_docblocks->p_Pop();
							t_docblocks->p_Push7(1);
						}else{
							if(m_parser->p_PopToken(String(L"on",2),false)){
								t_docswitch=1;
								t_doccdecl=(new c_DocDecl)->m_new(901,String());
								m_curparagraph=0;
							}else{
								t_doccdecl=(new c_DocDecl)->m_new(901,String());
								m_curparagraph=0;
							}
						}
					}
					m_parser->p_NextRestOfLine();
				}else{
					if(t_indoc){
						m_parser->p_Restore(t_state);
						String t_str=m_parser->p_NextRestOfLine();
						p_AppendDocContents(t_doccdecl,t_str);
						t_docblocks->p_Push7(0);
					}else{
						m_parser->p_NextRestOfLine();
						t_docblocks->p_Push7(1);
					}
				}
			}else{
				if((m_parser->p_NextCdata(String(L"#If",3))).Length()!=0){
					if(t_indoc){
						m_parser->p_Restore(t_state);
						String t_str2=m_parser->p_NextRestOfLine();
						p_AppendDocContents(t_doccdecl,t_str2);
						t_docblocks->p_Push7(0);
					}else{
						m_parser->p_NextRestOfLine();
						t_docblocks->p_Push7(2);
					}
				}else{
					if((m_parser->p_NextCdata(String(L"#End",4))).Length()!=0){
						t_docblocks->p_Pop();
						if(t_docblocks->p_Length2()>0 && t_docblocks->p_Get(0)==0){
							m_parser->p_Restore(t_state);
							String t_str3=m_parser->p_NextRestOfLine();
							p_AppendDocContents(t_doccdecl,t_str3);
						}
					}else{
						if((t_docswitch)!=0){
							if(t_indoc){
								m_parser->p_Restore(t_state);
								String t_str4=m_parser->p_NextRestOfLine();
								p_AppendDocContents(t_doccdecl,t_str4);
							}else{
								if(!(t_indoc || t_inrem)){
									m_parser->p_NextToke();
									m_parser->p_PopSpace(false);
									c_DocDecl* t_decl=m_parser->p_ParseDecl(t_curdecl);
									m_parser->p_NextRestOfLine();
									if((t_decl)!=0){
										t_curdecl=t_decl;
										if(((t_curdecl)!=0) && ((t_doccdecl)!=0) && t_doccdecl!=t_mdecl && ((t_doccdecl->m_childs)!=0)){
											c_Enumerator2* t_2=t_doccdecl->m_childs->p_ObjectEnumerator();
											while(t_2->p_HasNext()){
												c_DocDecl* t_d=t_2->p_NextObject();
												t_curdecl->p_Add(t_d);
											}
											t_doccdecl=0;
										}
									}
								}else{
									m_parser->p_NextRestOfLine();
								}
							}
						}else{
							m_parser->p_NextRestOfLine();
							if(!((t_mdecl)!=0) && m_parser->p_GetCarretLine()>5){
								return;
							}
						}
					}
				}
			}
		}
		if((t_mdecl)!=0){
			t_pScope->p_Add(t_mdecl);
		}
	}catch(c_ThrowableString* t_message){
		p_Error(t_message->p_ToString2());
	}
}
void c_ApiDoccer::p_ParseIn(String t_pDirectory,c_DocDecl* t_pScope,String t_pModulePath){
	Array<String > t_=bb_os_LoadDir(t_pDirectory,false,false);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_file=t_[t_2];
		t_2=t_2+1;
		String t_path=t_pDirectory+String(L"/",1)+t_file;
		String t_modpath=t_pModulePath;
		int t_1=FileType(t_path);
		if(t_1==2){
			if(t_file==String(L"3rdparty.cerberusdoc",20) || t_file==String(L"3rdparty.monkeydoc",18)){
				c_DocDecl* t_decl=(new c_DocDecl)->m_new(210,t_path);
				m_maker->m_branch3rdparty->p_Add(t_decl);
			}else{
				if(t_file==String(L"examples",8)){
					p_LoadExamples(t_path,t_pScope);
				}else{
					if(t_file==String(L"cerberusdoc",11)){
						String t_expath=t_path+String(L"/examples",9);
						if(FileType(t_expath)==2){
							p_LoadExamples(t_expath,t_pScope);
						}
					}else{
						if(!t_file.Contains(String(L".",1))){
							if((t_modpath).Length()!=0){
								t_modpath=t_modpath+String(L".",1)+t_file;
							}else{
								t_modpath=t_file;
							}
							if(!m_maker->m_ignoremods->p_Contains(t_modpath)){
								c_DocDecl* t_decl2=t_pScope->p_GetChild2(t_file);
								bool t_newscope=false;
								if(!((t_decl2)!=0)){
									t_decl2=(new c_DocDecl)->m_new(900,t_modpath);
									t_newscope=true;
									t_decl2->m_parent=t_pScope;
								}
								p_ParseIn(t_path,t_decl2,t_modpath);
								if(t_newscope && (((t_decl2->p_GetChilds2(901,true))!=0) || ((t_decl2->p_GetChild(200))!=0))){
									t_pScope->p_Add(t_decl2);
								}
							}
						}
					}
				}
			}
		}else{
			if(t_1==1){
				String t_name=bb_os_StripExt(t_file);
				String t_ext=bb_os_ExtractExt(t_file);
				String t_docdir=String();
				String t_docext=String();
				if(t_ext==String(L"cxs",3)){
					t_docdir=String(L"cerberusdoc/",12);
					t_docext=String(L".cerberusdoc",12);
				}else{
					if(t_ext==String(L"monkey",6)){
						t_docdir=String(L"monkeydoc/",10);
						t_docext=String(L".monkeydoc",10);
					}
				}
				if((t_docext).Length()!=0){
					c_DocDecl* t_tmpscope=0;
					c_DocDeclStack* t_scopechilds=0;
					if(t_name==bb_os_StripDir(t_pDirectory)){
						t_tmpscope=(new c_DocDecl)->m_new(901,String());
					}else{
						t_tmpscope=t_pScope;
						t_modpath=t_modpath+String(L".",1)+t_name;
						c_DocDecl* t_decl3=t_pScope->p_GetChild3(t_modpath,900);
						if((t_decl3)!=0){
							t_pScope->m_childs->p_RemoveEach(t_decl3);
							t_scopechilds=t_decl3->m_childs;
						}
					}
					if(!m_maker->m_ignoremods->p_Contains(t_modpath)){
						String t_dir=t_pDirectory+String(L"/",1);
						String t_p1=t_dir+t_name+t_docext;
						String t_p2=t_dir+t_docdir+t_name+t_docext;
						if(FileType(t_p1)==1){
							p_ParseDocFile(t_p1,t_tmpscope,t_modpath);
						}else{
							if((FileType(t_p2))!=0){
								p_ParseDocFile(t_p2,t_tmpscope,t_modpath);
								p_CopyDocsData(t_p2,t_modpath);
							}else{
								p_ParseSourceFile(t_path,t_tmpscope,t_modpath);
							}
						}
						if((t_scopechilds)!=0){
							c_DocDecl* t_mdecl=t_tmpscope->p_GetChild3(t_name,901);
							if((t_mdecl)!=0){
								c_Enumerator2* t_3=t_scopechilds->p_ObjectEnumerator();
								while(t_3->p_HasNext()){
									c_DocDecl* t_c=t_3->p_NextObject();
									t_mdecl->p_Add(t_c);
								}
							}
						}
						if(t_name==bb_os_StripDir(t_pDirectory)){
							c_DocDecl* t_mdecl2=t_tmpscope->p_GetChild(901);
							if(((t_mdecl2)!=0) && ((t_mdecl2->m_childs)!=0)){
								c_Enumerator2* t_4=t_mdecl2->m_childs->p_ObjectEnumerator();
								while(t_4->p_HasNext()){
									c_DocDecl* t_c2=t_4->p_NextObject();
									t_pScope->p_Add(t_c2);
								}
								t_pScope->m_kind=901;
								t_pScope->m_canbefound=true;
							}
						}
					}
				}
			}
		}
	}
}
void c_ApiDoccer::p_Parse(){
	Array<String > t_=m_maker->m_modpaths.Split(String(L";",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_path=t_[t_2];
		t_2=t_2+1;
		if(FileType(t_path)==2){
			p_ParseIn(t_path,m_maker->m_rootmodules,String());
		}
	}
}
String c_ApiDoccer::p_StripParagraph(String t_pText){
	if(t_pText.StartsWith(String(L"<p>",3)) && t_pText.EndsWith(String(L"</p>",4))){
		t_pText=t_pText.Slice(3,-4);
	}
	while(((t_pText).Length()!=0) && ((int)t_pText[0]==10 || (int)t_pText[0]==13)){
		t_pText=t_pText.Slice(1);
	}
	while(((t_pText).Length()!=0) && ((int)t_pText[t_pText.Length()-1]==10 || (int)t_pText[t_pText.Length()-1]==13)){
		t_pText=t_pText.Slice(0,-1);
	}
	return t_pText;
}
void c_ApiDoccer::p_SetPagerStrings(c_PageMaker* t_pPager,c_DocDecl* t_pDecl,bool t_pOnlyForIndex){
	c_DocDecl* t_tdecl=0;
	if((t_pDecl->m_target)!=0){
		t_tdecl=t_pDecl->m_target;
	}else{
		t_tdecl=t_pDecl;
	}
	String t_txt=String();
	t_pPager->p_SetString(String(L"IDENT",5),t_tdecl->p_GetIdent());
	t_pPager->p_SetString(String(L"URL",3),m_maker->p_BuildDocLink(t_tdecl,0));
	t_pPager->p_SetString(String(L"SCOPE",5),t_tdecl->p_GetScopeIdent());
	t_txt=t_tdecl->p_GetTextOfChild(850);
	t_txt=p_StripParagraph(m_maker->m_marker->p_ToHtml(t_txt));
	t_pPager->p_SetString(String(L"SUMMARY",7),t_txt);
	if(t_pOnlyForIndex){
		return;
	}
	t_txt=t_pDecl->p_GetKindName();
	t_pPager->p_SetString(String(L"KIND",4),t_txt);
	t_pPager->p_SetString(String(L"UIDENT",6),t_tdecl->p_GetIdentWithParams());
	t_txt=t_tdecl->p_GetTextOfChild(202);
	t_pPager->p_SetString(String(L"PARAMTYPES",10),t_txt);
	if(t_pDecl->m_kind==401){
		t_txt=bb_stringutil_HtmlEscape(t_pDecl->p_GetDocType(false));
		t_pPager->p_SetString(String(L"DECL",4),t_txt);
		t_pPager->p_SetString(String(L"XDECL",5),t_txt);
		t_pPager->p_SetString(String(L"TYPE",4),t_txt);
		t_pPager->p_SetString(String(L"XTYPE",5),t_txt);
	}else{
		if(t_tdecl->m_kind==301){
			t_txt=bb_stringutil_HtmlEscape(t_tdecl->p_GetGenType());
			t_pPager->p_SetString(String(L"DECL",4),t_txt);
			t_pPager->p_SetString(String(L"XDECL",5),t_txt);
			t_pPager->p_SetString(String(L"TYPE",4),t_txt);
			t_pPager->p_SetString(String(L"XTYPE",5),t_txt);
		}else{
			String t_xtxt=String();
			t_txt=bb_stringutil_HtmlEscape(t_tdecl->p_GetDocType(false));
			t_xtxt=p_StripParagraph(m_maker->m_marker->p_ToHtml(t_tdecl->p_GetDocXType()));
			t_pPager->p_SetString(String(L"DECL",4),t_txt);
			t_pPager->p_SetString(String(L"XDECL",5),t_xtxt);
			t_txt=t_txt.Trim();
			if(t_txt.StartsWith(String(L":",1))){
				t_txt=t_txt.Slice(1).Trim();
			}
			t_xtxt=t_xtxt.Trim();
			if(t_xtxt.StartsWith(String(L":",1))){
				t_xtxt=t_xtxt.Slice(1).Trim();
			}
			t_pPager->p_SetString(String(L"TYPE",4),t_txt);
			t_pPager->p_SetString(String(L"XTYPE",5),t_txt);
		}
	}
	if(t_tdecl->m_kind==601){
		t_tdecl=t_tdecl->m_parent;
	}
	t_txt=t_tdecl->p_GetTextOfChild(800);
	t_pPager->p_SetString(String(L"DESCRIPTION",11),m_maker->m_marker->p_ToHtml(t_txt));
	t_txt=t_tdecl->p_GetTextOfChild(802);
	t_pPager->p_SetString(String(L"LINKS",5),m_maker->m_marker->p_ToHtml(t_txt));
	t_txt=t_tdecl->p_GetTextOfChild(801);
	t_pPager->p_SetString(String(L"EXAMPLE",7),m_maker->m_marker->p_ToHtml(t_txt));
	t_txt=t_tdecl->p_GetTextOfChild(803);
	t_pPager->p_SetString(String(L"PARAMETERS",10),m_maker->m_marker->p_ToHtml(t_txt));
	t_txt=t_tdecl->p_GetTextOfChild(804);
	t_pPager->p_SetString(String(L"RETURNS",7),m_maker->m_marker->p_ToHtml(t_txt));
	c_DocDecl* t_ex=t_tdecl->p_GetChild(801);
	if((t_ex)!=0){
		t_txt=String(L"examples/",9)+t_ex->p_GetTextOfChild(222);
	}else{
		t_txt=String();
	}
	t_pPager->p_SetString(String(L"EXAMPLE_URL",11),t_txt);
	c_DocDeclStack* t_decls=0;
	t_decls=t_pDecl->p_GetChilds2(401,false);
	t_txt=String();
	if((t_decls)!=0){
		c_Enumerator2* t_=t_decls->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			if((t_txt).Length()!=0){
				t_txt=t_txt+String(L", ",2);
			}
			t_txt=t_txt+(String(L"[[",2)+t_d->p_GetTargetIdent()+String(L"]]",2)+bb_stringutil_HtmlEscape(t_d->p_GetType(false,true)));
		}
	}
	t_pPager->p_SetString(String(L"XEXTENDS",8),m_maker->m_marker->p_ToHtml(t_txt));
	t_decls=t_pDecl->p_GetChilds2(402,false);
	t_txt=String();
	if((t_decls)!=0){
		c_Enumerator2* t_2=t_decls->p_ObjectEnumerator();
		while(t_2->p_HasNext()){
			c_DocDecl* t_d2=t_2->p_NextObject();
			if((t_txt).Length()!=0){
				t_txt=t_txt+String(L", ",2);
			}
			t_txt=t_txt+(String(L"[[",2)+t_d2->p_GetTargetIdent()+String(L"]]",2));
		}
	}
	t_pPager->p_SetString(String(L"XIMPLEMENTS",11),m_maker->m_marker->p_ToHtml(t_txt));
	t_decls=t_pDecl->p_GetChilds2(701,false);
	t_txt=String();
	if((t_decls)!=0){
		c_Enumerator2* t_3=t_decls->p_ObjectEnumerator();
		while(t_3->p_HasNext()){
			c_DocDecl* t_d3=t_3->p_NextObject();
			if((t_txt).Length()!=0){
				t_txt=t_txt+String(L", ",2);
			}
			t_txt=t_txt+(String(L"[[",2)+t_d3->m_target->m_ident+String(L"]]",2)+bb_stringutil_HtmlEscape(t_d3->p_GetType(false,true)));
		}
	}
	t_pPager->p_SetString(String(L"XEXTENDED_BY",12),m_maker->m_marker->p_ToHtml(t_txt));
}
void c_ApiDoccer::p_SetPagerList(c_PageMaker* t_pPager,c_DocDecl* t_pDecl,int t_pKind,String t_pList){
	c_DocDeclStack* t_decls=t_pDecl->p_GetChilds2(t_pKind,false);
	if((t_decls)!=0){
		t_pPager->p_BeginList(t_pList);
		c_Enumerator2* t_=t_decls->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			t_pPager->p_AddItem();
			p_SetPagerStrings(t_pPager,t_d,false);
			int t_2=t_pKind;
			if(t_2==322 || t_2==422 || t_2==472){
				int t_kind=601;
				String t_listname=String(L"ENUM_ELEMENTS",13);
				c_DocDecl* t_tdecl=t_d;
				if((t_tdecl->m_target)!=0){
					t_tdecl=t_tdecl->m_target;
				}
				p_SetPagerList(t_pPager,t_tdecl,t_kind,t_listname);
			}
		}
		t_pPager->p_EndList();
	}
}
void c_ApiDoccer::p_SetPagerLists(c_PageMaker* t_pPager,c_DocDecl* t_pDecl,Array<int > t_pKinds,Array<String > t_pLists){
	for(int t_li=0;t_li<t_pKinds.Length();t_li=t_li+1){
		p_SetPagerList(t_pPager,t_pDecl,t_pKinds[t_li],t_pLists[t_li]);
	}
}
String c_ApiDoccer::p_ApplyModuleTemplate(c_DocDecl* t_pDecl,c_PageMaker* t_pPager){
	t_pPager->p_Clear();
	p_SetPagerStrings(t_pPager,t_pDecl,false);
	int t_[]={300,700,301,302,310,320,321,322};
	Array<int > t_kinds=Array<int >(t_,8);
	String t_2[]={String(L"IMPORTS",7),String(L"IMPORTED_BY",11),String(L"CLASSES",7),String(L"INTERFACES",10),String(L"FUNCTIONS",9),String(L"CONSTS",6),String(L"GLOBALS",7),String(L"ENUMS",5)};
	Array<String > t_lists=Array<String >(t_2,8);
	p_SetPagerLists(t_pPager,t_pDecl,t_kinds,t_lists);
	return t_pPager->p_MakePage();
}
String c_ApiDoccer::p_ApplyClassTemplate(c_DocDecl* t_pDecl,c_PageMaker* t_pPager){
	t_pPager->p_Clear();
	p_SetPagerStrings(t_pPager,t_pDecl,false);
	int t_[]={401,402,701,702,403,405,406,410,420,421,422,423,453,455,456,460,470,471,472,473};
	Array<int > t_kinds=Array<int >(t_,20);
	String t_2[]={String(L"EXTENDS",7),String(L"IMPLEMENTS",10),String(L"EXTENDED_BY",11),String(L"IMPLEMENTED_BY",14),String(L"METHODS",7),String(L"PROPERTIES",10),String(L"CTORS",5),String(L"FUNCTIONS",9),String(L"CONSTS",6),String(L"GLOBALS",7),String(L"ENUMS",5),String(L"FIELDS",6),String(L"INHERITED_METHODS",17),String(L"INHERITED_PROPERTIES",20),String(L"INHERITED_CTORS",15),String(L"INHERITED_FUNCTIONS",19),String(L"INHERITED_CONSTS",16),String(L"INHERITED_GLOBALS",17),String(L"INHERITED_ENUMS",15),String(L"INHERITED_FIELDS",16)};
	Array<String > t_lists=Array<String >(t_2,20);
	p_SetPagerLists(t_pPager,t_pDecl,t_kinds,t_lists);
	return t_pPager->p_MakePage();
}
void c_ApiDoccer::mark(){
	Object::mark();
}
String bb_os_ExtractExt(String t_path){
	int t_i=t_path.FindLast(String(L".",1));
	if(t_i!=-1 && t_path.Find(String(L"/",1),t_i+1)==-1 && t_path.Find(String(L"\\",1),t_i+1)==-1){
		return t_path.Slice(t_i+1);
	}
	return String();
}
String bb_os_StripExt(String t_path){
	int t_i=t_path.FindLast(String(L".",1));
	if(t_i!=-1 && t_path.Find(String(L"/",1),t_i+1)==-1 && t_path.Find(String(L"\\",1),t_i+1)==-1){
		return t_path.Slice(0,t_i);
	}
	return t_path;
}
String bb_os_StripDir(String t_path){
	int t_i=t_path.FindLast(String(L"/",1));
	if(t_i==-1){
		t_i=t_path.FindLast(String(L"\\",1));
	}
	if(t_i!=-1){
		return t_path.Slice(t_i+1);
	}
	return t_path;
}
c_Toker::c_Toker(){
	m__path=String();
	m__line=0;
	m__source=String();
	m__length=0;
	m__toke=String();
	m__tokeType=0;
	m__tokePos=0;
}
c_StringSet* c_Toker::m__keywords;
c_StringSet* c_Toker::m__symbols;
int c_Toker::p__init(){
	if((m__keywords)!=0){
		return 0;
	}
	m__keywords=(new c_StringSet)->m_new();
	Array<String > t_=String(L"void strict public private protected friend property bool int float string array object mod continue exit include import module extern new self super eachin true false null not extends abstract final select case default const local global field method function class and or shl shr end if then else elseif endif while wend repeat until forever for to step next return interface implements inline alias try catch throw throwable enumerate",437).Split(String(L" ",1));
	int t_2=0;
	while(t_2<t_.Length()){
		String t_t=t_[t_2];
		t_2=t_2+1;
		m__keywords->p_Insert(t_t);
	}
	m__symbols=(new c_StringSet)->m_new();
	m__symbols->p_Insert(String(L"..",2));
	m__symbols->p_Insert(String(L":=",2));
	m__symbols->p_Insert(String(L"*=",2));
	m__symbols->p_Insert(String(L"/=",2));
	m__symbols->p_Insert(String(L"+=",2));
	m__symbols->p_Insert(String(L"-=",2));
	m__symbols->p_Insert(String(L"|=",2));
	m__symbols->p_Insert(String(L"&=",2));
	m__symbols->p_Insert(String(L"~=",2));
	return 0;
}
c_Toker* c_Toker::m_new(String t_path,String t_source){
	p__init();
	m__path=t_path;
	m__line=1;
	m__source=t_source;
	m__length=m__source.Length();
	m__toke=String();
	m__tokeType=0;
	m__tokePos=0;
	return this;
}
c_Toker* c_Toker::m_new2(c_Toker* t_toker){
	p__init();
	m__path=t_toker->m__path;
	m__line=t_toker->m__line;
	m__source=t_toker->m__source;
	m__length=m__source.Length();
	m__toke=t_toker->m__toke;
	m__tokeType=t_toker->m__tokeType;
	m__tokePos=t_toker->m__tokePos;
	return this;
}
c_Toker* c_Toker::m_new3(){
	return this;
}
int c_Toker::p_TCHR(int t_i){
	t_i+=m__tokePos;
	if(t_i<m__length){
		return (int)m__source[t_i];
	}
	return 0;
}
String c_Toker::p_TSTR(int t_i){
	t_i+=m__tokePos;
	if(t_i<m__length){
		return m__source.Slice(t_i,t_i+1);
	}
	return String();
}
String c_Toker::p_NextToke(){
	m__toke=String();
	if(m__tokePos==m__length){
		m__tokeType=0;
		return m__toke;
	}
	int t_chr=p_TCHR(0);
	String t_str=p_TSTR(0);
	int t_start=m__tokePos;
	m__tokePos+=1;
	if(t_str==String(L"\n",1)){
		m__tokeType=11;
		m__line+=1;
	}else{
		if(t_str==String(L"\r",1)){
			if(p_TSTR(0)==String(L"\n",1)){
				m__tokePos+=1;
			}
			m__tokeType=11;
			m__line+=1;
		}else{
			if(bb_stringutil_IsSpace(t_chr)){
				m__tokeType=1;
				while(m__tokePos<m__length && bb_stringutil_IsSpace(p_TCHR(0)) && p_TSTR(0)!=String(L"\n",1)){
					m__tokePos+=1;
				}
			}else{
				if(t_str==String(L"_",1) || bb_stringutil_IsAlpha(t_chr)){
					m__tokeType=2;
					while(m__tokePos<m__length){
						int t_chr2=(int)m__source[m__tokePos];
						if(t_chr2!=95 && !bb_stringutil_IsAlpha(t_chr2) && !bb_stringutil_IsDigit(t_chr2)){
							break;
						}
						m__tokePos+=1;
					}
					m__toke=m__source.Slice(t_start,m__tokePos);
					if(m__keywords->p_Contains(m__toke.ToLower())){
						m__tokeType=3;
					}
				}else{
					if(bb_stringutil_IsDigit(t_chr) || t_str==String(L".",1) && bb_stringutil_IsDigit(p_TCHR(0))){
						m__tokeType=4;
						if(t_str==String(L".",1)){
							m__tokeType=5;
						}
						while(bb_stringutil_IsDigit(p_TCHR(0))){
							m__tokePos+=1;
						}
						if(m__tokeType==4 && p_TSTR(0)==String(L".",1) && bb_stringutil_IsDigit(p_TCHR(1))){
							m__tokeType=5;
							m__tokePos+=2;
							while(bb_stringutil_IsDigit(p_TCHR(0))){
								m__tokePos+=1;
							}
						}
						if(p_TSTR(0).ToLower()==String(L"e",1)){
							m__tokeType=5;
							m__tokePos+=1;
							if(p_TSTR(0)==String(L"+",1) || p_TSTR(0)==String(L"-",1)){
								m__tokePos+=1;
							}
							while(bb_stringutil_IsDigit(p_TCHR(0))){
								m__tokePos+=1;
							}
						}
					}else{
						if(t_str==String(L"%",1) && bb_stringutil_IsBinDigit(p_TCHR(0))){
							m__tokeType=4;
							m__tokePos+=1;
							while(bb_stringutil_IsBinDigit(p_TCHR(0))){
								m__tokePos+=1;
							}
						}else{
							if(t_str==String(L"$",1) && bb_stringutil_IsHexDigit(p_TCHR(0))){
								m__tokeType=4;
								m__tokePos+=1;
								while(bb_stringutil_IsHexDigit(p_TCHR(0))){
									m__tokePos+=1;
								}
							}else{
								if(t_str==String(L"\"",1)){
									m__tokeType=6;
									while(m__tokePos<m__length && p_TSTR(0)!=String(L"\"",1)){
										m__tokePos+=1;
									}
									if(m__tokePos<m__length){
										m__tokePos+=1;
									}else{
										m__tokeType=7;
									}
								}else{
									if(t_str==String(L"`",1)){
										m__tokeType=4;
										while(m__tokePos<m__length && p_TSTR(0)!=String(L"`",1)){
											m__tokePos+=1;
										}
										if(m__tokePos<m__length){
											m__tokePos+=1;
										}else{
											m__tokeType=10;
										}
									}else{
										if(t_str==String(L"'",1)){
											m__tokeType=9;
											while(m__tokePos<m__length && p_TSTR(0)!=String(L"\n",1)){
												m__tokePos+=1;
											}
											if(m__tokePos<m__length){
												m__tokePos+=1;
												m__line+=1;
											}
										}else{
											m__tokeType=8;
											if(m__symbols->p_Contains(m__source.Slice(m__tokePos-1,m__tokePos+1))){
												m__tokePos+=1;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if(!((m__toke).Length()!=0)){
		m__toke=m__source.Slice(t_start,m__tokePos);
	}
	return m__toke;
}
void c_Toker::mark(){
	Object::mark();
}
c_Parser::c_Parser(){
}
c_Parser* c_Parser::m_new(String t_pText){
	c_Toker::m_new(String(),t_pText);
	p_NextToke();
	return this;
}
c_Parser* c_Parser::m_new2(){
	c_Toker::m_new3();
	return this;
}
c_Toker* c_Parser::p_Store(){
	c_Toker* t_t=(new c_Toker)->m_new3();
	t_t->m__line=m__line;
	t_t->m__toke=m__toke;
	t_t->m__tokeType=m__tokeType;
	t_t->m__tokePos=m__tokePos;
	return t_t;
}
String c_Parser::p_NextSpace(){
	m__toke=String();
	if(m__tokePos==m__length){
		m__tokeType=0;
		return m__toke;
	}
	int t_start=m__tokePos;
	do{
		int t_c=p_TCHR(0);
		if(t_c!=32 && t_c!=9){
			break;
		}
		m__tokePos+=1;
	}while(!(false));
	m__toke=m__source.Slice(t_start,m__tokePos);
	m__tokeType=1;
	return m__toke;
}
String c_Parser::p_NextCdata(String t_pString){
	m__toke=String();
	if(m__tokePos==m__length){
		m__tokeType=0;
		return m__toke;
	}
	int t_ln=t_pString.Length();
	if(m__tokePos+t_ln<m__length){
		String t_sstr=m__source.Slice(m__tokePos,m__tokePos+t_ln);
		if(t_sstr==t_pString){
			m__toke=t_sstr;
			m__tokeType=12;
			m__tokePos+=t_ln;
			return m__toke;
		}
	}
	return m__toke;
}
void c_Parser::p_Pop(){
	p_NextToke();
}
void c_Parser::p_Error(String t_pMessage){
	throw (new c_ThrowableString)->m_new(t_pMessage);
}
void c_Parser::p_PopSpace(bool t_pAlsoLineBreak){
	do{
		if(m__tokeType==0){
			break;
		}else{
			if(m__tokeType==9 || m__tokeType==11){
				if(!t_pAlsoLineBreak){
					break;
				}
			}else{
				if(m__tokeType!=1){
					break;
				}
			}
		}
		p_NextToke();
	}while(!(false));
}
void c_Parser::p_Restore(c_Toker* t_toker){
	m__line=t_toker->m__line;
	m__toke=t_toker->m__toke;
	m__tokeType=t_toker->m__tokeType;
	m__tokePos=t_toker->m__tokePos;
}
bool c_Parser::p__PopToken(String t_pString,int t_pType,bool t_pNewline,bool t_pPop,bool t_pCaseSensitive){
	c_Toker* t_state=p_Store();
	p_PopSpace(t_pNewline);
	String t_ltoke=m__toke;
	String t_lstr=t_pString;
	if(!t_pCaseSensitive){
		t_ltoke=t_ltoke.ToLower();
		t_lstr=t_lstr.ToLower();
	}
	if((t_lstr==String() || t_ltoke==t_lstr) && (t_pType==-1 || m__tokeType==t_pType)){
		if(t_pPop){
			p_Pop();
		}
		return true;
	}
	p_Restore(t_state);
	return false;
}
bool c_Parser::p_PopUntilToken(String t_pString,bool t_pNewline){
	return p__PopToken(t_pString,-1,t_pNewline,false,true);
}
bool c_Parser::p_PopUntilToken2(int t_pType,bool t_pNewline){
	return p__PopToken(String(),t_pType,t_pNewline,false,true);
}
bool c_Parser::p_PopToken(String t_pString,bool t_pNewline){
	return p__PopToken(t_pString,-1,t_pNewline,true,true);
}
bool c_Parser::p_PopToken2(int t_pType,bool t_pNewline){
	return p__PopToken(String(),t_pType,t_pNewline,true,true);
}
String c_Parser::p_SParseModpath(bool t_pMayEndInKeyword){
	c_Toker* t_state=p_Store();
	String t_str=String();
	do{
		if(p_PopUntilToken2(2,false)){
			t_str=t_str+m__toke;
			p_Pop();
		}else{
			if(t_pMayEndInKeyword && p_PopUntilToken2(3,false)){
				t_str=t_str+m__toke;
				p_Pop();
				return t_str;
			}else{
				p_Restore(t_state);
				return String();
			}
		}
		if(p_PopToken(String(L".",1),false)){
			t_str=t_str+String(L".",1);
		}else{
			return t_str;
		}
	}while(!(false));
}
c_DocDecl* c_Parser::p_ParseModuleHeader(c_DocDecl* t_pScope){
	if(t_pScope->m_kind!=901){
		p_Error(String(L"Module header must be at module scope",37));
	}
	c_DocDecl* t_decl=0;
	String t_str=p_SParseModpath(false);
	if(t_str==t_pScope->m_ident || t_pScope->m_ident.EndsWith(String(L".",1)+t_str)){
		t_decl=(new c_DocDecl)->m_new(200,String());
	}else{
		if(!((t_str).Length()!=0)){
			p_Error(String(L"Expecting modpath",17));
		}else{
			p_Error(String(L"Module header does not match modpath",36));
		}
	}
	t_pScope->p_Add(t_decl);
	return t_decl;
}
c_DocDecl* c_Parser::p_ParseImportDecl(c_DocDecl* t_pScope){
	if(t_pScope->m_kind!=901){
		p_Error(String(L"Import declaration must be at module scope",42));
	}
	c_DocDecl* t_decl=0;
	String t_str=p_SParseModpath(false);
	if((t_str).Length()!=0){
		t_decl=(new c_DocDecl)->m_new(300,t_str);
	}else{
		p_Error(String(L"Expecting modpath",17));
	}
	t_pScope->p_Add(t_decl);
	return t_decl;
}
c_DocDecl* c_Parser::p_GetModuleScope(c_DocDecl* t_pDecl){
	return t_pDecl->p_GetScope(901);
}
String c_Parser::p_SParseClasspath(){
	return p_SParseModpath(true);
}
void c_Parser::p_PopLineBreak(){
	p_PopSpace(true);
}
c_Stack2* c_Parser::p_ParseTypeParameters(c_DocDecl* t_pScope){
	if(p_PopToken(String(L"<",1),false)){
		c_DocDecl* t_decl=0;
		c_Stack2* t_decls=(new c_Stack2)->m_new();
		do{
			if(p_PopUntilToken2(2,false)){
				t_decl=(new c_DocDecl)->m_new(400,m__toke);
				t_decls->p_Push4(t_decl);
				p_Pop();
			}else{
				p_Error(String(L"Expecting type parameter identifier",35));
			}
			if(p_PopToken(String(L">",1),false)){
				t_pScope->p_Add2(t_decls);
				return t_decls;
			}else{
				if(p_PopToken(String(L",",1),false)){
					p_PopLineBreak();
				}
			}
		}while(!(false));
	}
	return 0;
}
bool c_Parser::p_PopKeyword(String t_pString,bool t_pNewline){
	return p__PopToken(t_pString,3,t_pNewline,true,false);
}
String c_Parser::p_SParseExpression(){
	c_Toker* t_state=p_Store();
	String t_str=String();
	c_StringStack* t_brackets=(new c_StringStack)->m_new2();
	bool t_instr=false;
	bool t_linebreakallowed=false;
	m__tokePos-=m__toke.Length();
	while(m__tokePos<m__length){
		int t_c=p_TCHR(0);
		String t_s=p_TSTR(0);
		if(t_c==34){
			t_instr=!t_instr;
		}
		if(t_instr){
			t_str=t_str+t_s;
		}else{
			if(t_c==91 || t_c==40 || t_c==60){
				t_brackets->p_Push(t_s);
				t_linebreakallowed=true;
				t_str=t_str+t_s;
			}else{
				if(t_c==93 || t_c==41 || t_c==62){
					if(t_brackets->p_IsEmpty()){
						p_NextToke();
						return t_str;
					}
					String t_obrack=t_brackets->p_Get(0);
					if(t_c==93 && t_obrack==String(L"[",1) || t_c==41 && t_obrack==String(L"(",1) || t_c==62 && t_obrack==String(L"<",1)){
						t_brackets->p_Pop();
					}else{
						p_Error(String(L"Unexpected '",12)+t_s+String(L"'",1));
					}
					t_linebreakallowed=false;
					t_str=t_str+t_s;
				}else{
					if(t_c==39){
						p_NextToke();
						if(!t_linebreakallowed){
							return t_str;
						}
						if(((t_str).Length()!=0) && (int)t_str.Slice(-1)[0]>32){
							t_str=t_str+String(L" ",1);
						}
					}else{
						if(t_c==10 || t_c==13){
							p_NextToke();
							if(!t_linebreakallowed){
								return t_str;
							}
							if(((t_str).Length()!=0) && (int)t_str.Slice(-1)[0]>32){
								t_str=t_str+String(L" ",1);
							}
						}else{
							if(t_c==44){
								if(t_brackets->p_IsEmpty()){
									p_NextToke();
									return t_str;
								}
								t_linebreakallowed=true;
							}else{
								if(t_c==59){
									p_NextToke();
									return t_str;
								}else{
									if(t_c<=32){
										if(((t_str).Length()!=0) && (int)t_str.Slice(-1)[0]>32){
											t_str=t_str+String(L" ",1);
										}
									}else{
										t_str=t_str+t_s;
									}
									t_linebreakallowed=false;
								}
							}
						}
					}
				}
			}
			String t_e1=m__source.Slice(-1);
			String t_e2=m__source.Slice(-2).ToLower();
			String t_e3=m__source.Slice(-3).ToLower();
			if(t_e1==String(L".",1) || t_e1==String(L"+",1) || t_e1==String(L"-",1) || t_e1==String(L"~",1) || t_e3==String(L"not",3) || t_e1==String(L"*",1) || t_e1==String(L"/",1) || t_e3==String(L"mod",3) || t_e3==String(L"shl",3) || t_e3==String(L"shr",3) || t_e1==String(L"&",1) || t_e1==String(L"|",1) || t_e1==String(L"=",1) || t_e1==String(L"<",1) || t_e1==String(L">",1) || t_e3==String(L"and",3) || t_e2==String(L"or",2)){
				t_linebreakallowed=true;
			}
		}
		m__tokePos+=1;
	}
	return String();
}
c_DocDecl* c_Parser::p_ParseTypeArray(c_DocDecl* t_pScope){
	if(p_PopToken(String(L"[",1),false)){
		p_SParseExpression();
		if(p_PopToken(String(L"]",1),false)){
			c_DocDecl* t_decl=(new c_DocDecl)->m_new(550,String());
			t_pScope->p_Add(t_decl);
			return t_decl;
		}else{
			p_Error(String(L"Expecting `]`",13));
		}
	}
	return 0;
}
c_DocDecl* c_Parser::p_ParseType(c_DocDecl* t_pScope){
	if(p_PopUntilToken2(2,false) || p_PopUntilToken2(3,false)){
		c_DocDecl* t_decl=(new c_DocDecl)->m_new(500,m__toke);
		p_Pop();
		do{
			if(p_PopUntilToken(String(L"[",1),false)){
				p_ParseTypeArray(t_decl);
			}else{
				if(p_PopUntilToken(String(L"<",1),false)){
					p_ParseTypeArguments(t_decl);
				}else{
					break;
				}
			}
		}while(!(false));
		t_pScope->p_Add(t_decl);
		return t_decl;
	}
	return 0;
}
c_Stack2* c_Parser::p_ParseTypeArguments(c_DocDecl* t_pScope){
	if(p_PopToken(String(L"<",1),false)){
		c_DocDecl* t_decl=0;
		c_DocDecl* t_arg=0;
		c_Stack2* t_args=(new c_Stack2)->m_new();
		do{
			t_arg=(new c_DocDecl)->m_new(551,String());
			if((p_ParseType(t_arg))!=0){
				t_arg->m_ident=t_arg->m_childs->p_Get(0)->m_ident;
				t_arg->m_childs=0;
				t_args->p_Push4(t_arg);
			}else{
				p_Error(String(L"Expecting type argument",23));
			}
			if(p_PopToken(String(L">",1),false)){
				t_pScope->p_Add2(t_args);
				return t_args;
			}else{
				if(p_PopToken(String(L",",1),false)){
					p_PopLineBreak();
				}
			}
		}while(!(false));
	}
	return 0;
}
c_DocDecl* c_Parser::p_ParseClassExtends(c_DocDecl* t_pScope){
	if(p_PopKeyword(String(L"extends",7),false)){
		c_DocDecl* t_decl=0;
		String t_str=p_SParseClasspath();
		if((t_str).Length()!=0){
			t_decl=(new c_DocDecl)->m_new(401,t_str);
		}else{
			p_Error(String(L"Expecting base class",20));
		}
		p_ParseTypeArguments(t_decl);
		t_pScope->p_Add(t_decl);
		return t_decl;
	}
	return 0;
}
c_Stack2* c_Parser::p_ParseClassImplements(c_DocDecl* t_pScope,String t_pKeyword){
	if(p_PopKeyword(t_pKeyword,false)){
		String t_str=String();
		c_DocDecl* t_decl=0;
		c_Stack2* t_decls=(new c_Stack2)->m_new();
		do{
			t_str=p_SParseModpath(false);
			if((t_str).Length()!=0){
				t_decl=(new c_DocDecl)->m_new(401,t_str);
				t_decls->p_Push4(t_decl);
			}else{
				p_Error(String(L"Expecting base interface",24));
			}
			if(p_PopToken(String(L",",1),false)){
				p_PopLineBreak();
			}else{
				t_pScope->p_Add2(t_decls);
				return t_decls;
			}
		}while(!(false));
	}
	return 0;
}
c_Stack2* c_Parser::p_ParseInterfaceExtends(c_DocDecl* t_pScope){
	return p_ParseClassImplements(t_pScope,String(L"extends",7));
}
c_DocDecl* c_Parser::p_ParseClassDecl(c_DocDecl* t_pScope,int t_pKind){
	c_Toker* t_state=p_Store();
	String t_ident=p_SParseClasspath();
	if(!((t_ident).Length()!=0)){
		p_Error(String(L"Expecting class identifier",26));
	}
	if(t_ident.Contains(String(L".",1))){
		int t_i=t_ident.FindLast(String(L".",1));
		t_ident=t_ident.Slice(t_i+1);
	}
	c_DocDecl* t_decl=(new c_DocDecl)->m_new(t_pKind,t_ident);
	if(t_pKind==301){
		p_ParseTypeParameters(t_decl);
		p_ParseClassExtends(t_decl);
		p_ParseClassImplements(t_decl,String(L"implements",10));
	}else{
		if(t_pKind==302){
			p_ParseInterfaceExtends(t_decl);
		}
	}
	t_pScope->p_Add(t_decl);
	return t_decl;
}
c_DocDecl* c_Parser::p_GetClassScope(c_DocDecl* t_pDecl){
	c_DocDecl* t_scope=0;
	t_scope=t_pDecl->p_GetScope(301);
	if(!((t_scope)!=0)){
		t_scope=t_pDecl->p_GetScope(302);
	}
	return t_scope;
}
c_DocDecl* c_Parser::p_ParseTypeDecl(c_DocDecl* t_pScope,int t_pAllowImplicitTyping){
	c_DocDecl* t_decl=0;
	if(p_PopToken(String(L"?",1),false)){
		t_decl=(new c_DocDecl)->m_new(500,String(L"Bool",4));
		p_ParseTypeArray(t_decl);
	}else{
		if(p_PopToken(String(L"%",1),false)){
			t_decl=(new c_DocDecl)->m_new(500,String(L"Int",3));
			p_ParseTypeArray(t_decl);
		}else{
			if(p_PopToken(String(L"#",1),false)){
				t_decl=(new c_DocDecl)->m_new(500,String(L"Float",5));
				p_ParseTypeArray(t_decl);
			}else{
				if(p_PopToken(String(L"$",1),false)){
					t_decl=(new c_DocDecl)->m_new(500,String(L"String",6));
					p_ParseTypeArray(t_decl);
				}else{
					if(p_PopUntilToken(String(L"[",1),false) || p_PopUntilToken(String(L"(",1),false) || p_PopUntilToken(String(L"=",1),false) || p_PopUntilToken(String(L",",1),false)){
						t_decl=(new c_DocDecl)->m_new(500,String(L"Int",3));
						p_ParseTypeArray(t_decl);
					}else{
						if(p_PopUntilToken(String(L":=",2),false) && ((t_pAllowImplicitTyping)!=0)){
							t_decl=(new c_DocDecl)->m_new(501,String());
							m__tokePos-=1;
							p_NextToke();
						}else{
							if(p_PopToken(String(L":",1),false)){
								t_decl=p_ParseType(t_pScope);
								return t_decl;
							}else{
								p_Error(String(L"Expecting type declaration",26));
							}
						}
					}
				}
			}
		}
	}
	t_pScope->p_Add(t_decl);
	return t_decl;
}
c_DocDecl* c_Parser::p_ParseInitialValue(c_DocDecl* t_pScope){
	if(p_PopToken(String(L"=",1),false)){
		String t_str=p_SParseExpression().Trim();
		if((t_str).Length()!=0){
			c_DocDecl* t_decl=(new c_DocDecl)->m_new(502,t_str);
			t_pScope->p_Add(t_decl);
			return t_decl;
		}else{
			p_Error(String(L"Expecting expression",20));
		}
	}
	return 0;
}
c_DocDecl* c_Parser::p_ParseVariable(c_DocDecl* t_pScope,int t_pKind){
	if(p_PopUntilToken2(2,false)){
		c_DocDecl* t_decl=(new c_DocDecl)->m_new(t_pKind,m__toke);
		p_Pop();
		c_DocDecl* t_tdecl=p_ParseTypeDecl(t_decl,1);
		if(t_tdecl->m_kind==501){
			if(!((p_ParseInitialValue(t_decl))!=0)){
				p_Error(String(L"Expecting initial expression",28));
			}
		}else{
			p_ParseInitialValue(t_decl);
		}
		t_pScope->p_Add(t_decl);
		return t_decl;
	}
	return 0;
}
c_Stack2* c_Parser::p_ParseVariableSet(c_DocDecl* t_pScope,int t_pKind){
	c_Stack2* t_decls=(new c_Stack2)->m_new();
	do{
		c_DocDecl* t_decl=p_ParseVariable(t_pScope,t_pKind);
		if((t_decl)!=0){
			t_decls->p_Push4(t_decl);
		}else{
			p_Error(String(L"Expecting variable identifier",29));
		}
		if(p_PopToken(String(L",",1),false)){
			p_PopLineBreak();
		}else{
			return t_decls;
		}
	}while(!(false));
}
c_Stack2* c_Parser::p_ParseFunctionParameters(c_DocDecl* t_pScope){
	if(p_PopToken(String(L"(",1),false)){
		if(p_PopToken(String(L")",1),false)){
			return 0;
		}
		c_Stack2* t_decls=p_ParseVariableSet(t_pScope,600);
		p_PopToken(String(L")",1),false);
		return t_decls;
	}
	return 0;
}
c_DocDecl* c_Parser::p_ParseFunctionDecl(c_DocDecl* t_pScope,int t_pKind){
	if(p_PopUntilToken2(2,false) || p_PopUntilToken2(3,false)){
		c_DocDecl* t_decl=0;
		if(m__toke==String(L"New",3)){
			t_decl=(new c_DocDecl)->m_new(406,m__toke);
			p_Pop();
		}else{
			t_decl=(new c_DocDecl)->m_new(t_pKind,m__toke);
			p_Pop();
			p_ParseTypeDecl(t_decl,0);
		}
		p_ParseFunctionParameters(t_decl);
		if(t_decl->m_kind==403){
			c_Toker* t_state=p_Store();
			if(p_PopKeyword(String(L"property",8),false)){
				t_decl->m_kind=405;
			}else{
				p_Restore(t_state);
			}
		}
		t_pScope->p_Add(t_decl);
		return t_decl;
	}else{
		p_Error(String(L"Expecting function identifier",29));
	}
	return 0;
}
c_DocDecl* c_Parser::p_ParseEnumDecl(c_DocDecl* t_pScope,int t_pKind){
	c_DocDecl* t_decl=(new c_DocDecl)->m_new(t_pKind,String());
	do{
		if(p_PopUntilToken2(2,false)){
			c_DocDecl* t_edecl=(new c_DocDecl)->m_new(601,m__toke);
			p_Pop();
			t_decl->p_Add(t_edecl);
			p_ParseInitialValue(t_edecl);
		}else{
			p_Error(String(L"Expecting identifier",20));
		}
		if(p_PopToken(String(L",",1),false)){
			p_PopLineBreak();
		}else{
			t_pScope->p_Add(t_decl);
			return t_decl;
		}
	}while(!(false));
}
c_DocDecl* c_Parser::p_ParseDecl(c_DocDecl* t_pScope){
	String t_1=m__toke.ToLower();
	if(t_1==String(L"module",6)){
		p_Pop();
		p_ParseModuleHeader(t_pScope);
		return t_pScope;
	}else{
		if(t_1==String(L"import",6)){
			p_Pop();
			p_ParseImportDecl(t_pScope);
			return t_pScope;
		}else{
			if(t_1==String(L"class",5)){
				p_Pop();
				return p_ParseClassDecl(p_GetModuleScope(t_pScope),301);
			}else{
				if(t_1==String(L"interface",9)){
					p_Pop();
					return p_ParseClassDecl(p_GetModuleScope(t_pScope),302);
				}else{
					if(t_1==String(L"function",8)){
						p_Pop();
						c_DocDecl* t_scope=p_GetClassScope(t_pScope);
						if((t_scope)!=0){
							return p_ParseFunctionDecl(t_scope,410);
						}else{
							t_scope=p_GetModuleScope(t_pScope);
							return p_ParseFunctionDecl(t_scope,310);
						}
					}else{
						if(t_1==String(L"method",6)){
							p_Pop();
							c_DocDecl* t_scope2=p_GetClassScope(t_pScope);
							if((t_scope2)!=0){
								return p_ParseFunctionDecl(t_scope2,403);
							}else{
								p_Error(String(L"Method declaration must be at class scope",41));
							}
						}else{
							if(t_1==String(L"const",5) || t_1==String(L"global",6)){
								String t_ltoke=m__toke.ToLower();
								p_Pop();
								c_DocDecl* t_scope3=p_GetClassScope(t_pScope);
								if((t_scope3)!=0){
									int t_kind=0;
									if(t_ltoke==String(L"const",5)){
										t_kind=420;
									}
									if(t_ltoke==String(L"global",6)){
										t_kind=421;
									}
									c_Stack2* t_decls=p_ParseVariableSet(t_scope3,t_kind);
									return t_decls->p_Get(0);
								}else{
									t_scope3=p_GetModuleScope(t_pScope);
									int t_kind2=0;
									if(t_ltoke==String(L"const",5)){
										t_kind2=320;
									}
									if(t_ltoke==String(L"global",6)){
										t_kind2=321;
									}
									c_Stack2* t_decls2=p_ParseVariableSet(t_scope3,t_kind2);
									return t_decls2->p_Get(0);
								}
							}else{
								if(t_1==String(L"enumerate",9)){
									p_Pop();
									if(t_pScope->m_kind==403 || t_pScope->m_kind==410 || t_pScope->m_kind==310){
										return 0;
									}
									c_DocDecl* t_scope4=p_GetClassScope(t_pScope);
									if((t_scope4)!=0){
										return p_ParseEnumDecl(t_scope4,422);
									}else{
										t_scope4=p_GetModuleScope(t_pScope);
										return p_ParseEnumDecl(t_scope4,322);
									}
								}else{
									if(t_1==String(L"field",5)){
										p_Pop();
										c_DocDecl* t_scope5=p_GetClassScope(t_pScope);
										if((t_scope5)!=0){
											c_Stack2* t_decls3=p_ParseVariableSet(t_scope5,423);
											return t_decls3->p_Get(0);
										}else{
											p_Error(String(L"Field declaration must be at class scope",40));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}
String c_Parser::p_NextRestOfLine(){
	if(m__tokeType==11 || m__tokeType==9){
		m__tokeType=12;
		return m__toke;
	}
	m__toke=String();
	if(m__tokePos==m__length){
		m__tokeType=0;
		return m__toke;
	}
	int t_start=m__tokePos;
	do{
		int t_c=p_TCHR(0);
		m__tokePos+=1;
		if(t_c==10){
			m__line+=1;
			break;
		}else{
			if(t_c==13){
				if(p_TCHR(0)==10){
					m__tokePos+=1;
				}
				m__line+=1;
				break;
			}else{
				if(t_c==0){
					m__tokePos-=1;
					break;
				}
			}
		}
	}while(!(false));
	m__toke=m__source.Slice(t_start,m__tokePos);
	m__tokeType=12;
	return m__toke;
}
int c_Parser::p_GetCarretLine(){
	return m__line;
}
int c_Parser::p_GetCarretChar(){
	int t_i=m__tokePos;
	do{
		int t_c=(int)m__source[t_i];
		if(t_c==10 || t_c==13){
			break;
		}
		t_i-=1;
	}while(!(t_i<0));
	return m__tokePos-t_i;
}
bool c_Parser::p_PopUntilKeyword(String t_pString,bool t_pNewline){
	return p__PopToken(t_pString,3,t_pNewline,false,false);
}
void c_Parser::mark(){
	c_Toker::mark();
}
c_Set::c_Set(){
	m_map=0;
}
c_Set* c_Set::m_new(c_Map* t_map){
	this->m_map=t_map;
	return this;
}
c_Set* c_Set::m_new2(){
	return this;
}
int c_Set::p_Insert(String t_value){
	m_map->p_Insert2(t_value,0);
	return 0;
}
bool c_Set::p_Contains(String t_value){
	return m_map->p_Contains(t_value);
}
void c_Set::mark(){
	Object::mark();
}
c_StringSet::c_StringSet(){
}
c_StringSet* c_StringSet::m_new(){
	c_Set::m_new((new c_StringMap)->m_new());
	return this;
}
void c_StringSet::mark(){
	c_Set::mark();
}
c_Map::c_Map(){
	m_root=0;
}
c_Map* c_Map::m_new(){
	return this;
}
int c_Map::p_RotateLeft(c_Node2* t_node){
	c_Node2* t_child=t_node->m_right;
	t_node->m_right=t_child->m_left;
	if((t_child->m_left)!=0){
		t_child->m_left->m_parent=t_node;
	}
	t_child->m_parent=t_node->m_parent;
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			t_node->m_parent->m_left=t_child;
		}else{
			t_node->m_parent->m_right=t_child;
		}
	}else{
		m_root=t_child;
	}
	t_child->m_left=t_node;
	t_node->m_parent=t_child;
	return 0;
}
int c_Map::p_RotateRight(c_Node2* t_node){
	c_Node2* t_child=t_node->m_left;
	t_node->m_left=t_child->m_right;
	if((t_child->m_right)!=0){
		t_child->m_right->m_parent=t_node;
	}
	t_child->m_parent=t_node->m_parent;
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			t_node->m_parent->m_right=t_child;
		}else{
			t_node->m_parent->m_left=t_child;
		}
	}else{
		m_root=t_child;
	}
	t_child->m_right=t_node;
	t_node->m_parent=t_child;
	return 0;
}
int c_Map::p_InsertFixup(c_Node2* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node2* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight(t_node->m_parent->m_parent);
			}
		}else{
			c_Node2* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map::p_Set(String t_key,Object* t_value){
	c_Node2* t_node=m_root;
	c_Node2* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare2(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				t_node->m_value=t_value;
				return false;
			}
		}
	}
	t_node=(new c_Node2)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			t_parent->m_right=t_node;
		}else{
			t_parent->m_left=t_node;
		}
		p_InsertFixup(t_node);
	}else{
		m_root=t_node;
	}
	return true;
}
bool c_Map::p_Insert2(String t_key,Object* t_value){
	return p_Set(t_key,t_value);
}
c_Node2* c_Map::p_FindNode(String t_key){
	c_Node2* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare2(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
bool c_Map::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
int c_Map::p_Clear(){
	m_root=0;
	return 0;
}
Object* c_Map::p_Get2(String t_key){
	c_Node2* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
void c_Map::mark(){
	Object::mark();
}
c_StringMap::c_StringMap(){
}
c_StringMap* c_StringMap::m_new(){
	c_Map::m_new();
	return this;
}
int c_StringMap::p_Compare2(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap::mark(){
	c_Map::mark();
}
c_Node2::c_Node2(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node2* c_Node2::m_new(String t_key,Object* t_value,int t_color,c_Node2* t_parent){
	this->m_key=t_key;
	this->m_value=t_value;
	this->m_color=t_color;
	this->m_parent=t_parent;
	return this;
}
c_Node2* c_Node2::m_new2(){
	return this;
}
void c_Node2::mark(){
	Object::mark();
}
bool bb_stringutil_IsSpace(int t_ch){
	return t_ch<=32;
}
bool bb_stringutil_IsAlpha(int t_ch){
	return t_ch>=65 && t_ch<=90 || t_ch>=97 && t_ch<=122;
}
bool bb_stringutil_IsDigit(int t_ch){
	return t_ch>=48 && t_ch<=57;
}
bool bb_stringutil_IsBinDigit(int t_ch){
	return t_ch==48 || t_ch==49;
}
bool bb_stringutil_IsHexDigit(int t_ch){
	return t_ch>=48 && t_ch<=57 || t_ch>=65 && t_ch<=70 || t_ch>=97 && t_ch<=102;
}
c_ThrowableString::c_ThrowableString(){
	m_str=String();
}
c_ThrowableString* c_ThrowableString::m_new(String t_pString){
	m_str=t_pString;
	return this;
}
c_ThrowableString* c_ThrowableString::m_new2(){
	return this;
}
String c_ThrowableString::p_ToString2(){
	return m_str;
}
void c_ThrowableString::mark(){
	ThrowableObject::mark();
}
String bb_stringutil_UnifyLineEndings(String t_str){
	return t_str.Replace(String(L"\r\n",2),String(L"\n",1)).Replace(String(L"\r",1),String(L"\n",1));
}
c_Stack3::c_Stack3(){
	m_data=Array<int >();
	m_length=0;
}
c_Stack3* c_Stack3::m_new(){
	return this;
}
c_Stack3* c_Stack3::m_new2(Array<int > t_data){
	this->m_data=t_data.Slice(0);
	this->m_length=t_data.Length();
	return this;
}
c_BackwardsStack* c_Stack3::p_Backwards(){
	return (new c_BackwardsStack)->m_new(this);
}
void c_Stack3::p_Push7(int t_value){
	if(m_length==m_data.Length()){
		m_data=m_data.Resize(m_length*2+10);
	}
	m_data[m_length]=t_value;
	m_length+=1;
}
void c_Stack3::p_Push8(Array<int > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push7(t_values[t_offset+t_i]);
	}
}
void c_Stack3::p_Push9(Array<int > t_values,int t_offset){
	p_Push8(t_values,t_offset,t_values.Length()-t_offset);
}
int c_Stack3::m_NIL;
int c_Stack3::p_Pop(){
	m_length-=1;
	int t_v=m_data[m_length];
	m_data[m_length]=m_NIL;
	return t_v;
}
void c_Stack3::p_Length(int t_newlength){
	if(t_newlength<m_length){
		for(int t_i=t_newlength;t_i<m_length;t_i=t_i+1){
			m_data[t_i]=m_NIL;
		}
	}else{
		if(t_newlength>m_data.Length()){
			m_data=m_data.Resize(bb_math_Max(m_length*2+10,t_newlength));
		}
	}
	m_length=t_newlength;
}
int c_Stack3::p_Length2(){
	return m_length;
}
int c_Stack3::p_Get(int t_index){
	return m_data[t_index];
}
void c_Stack3::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		m_data[t_i]=m_NIL;
	}
	m_length=0;
}
void c_Stack3::mark(){
	Object::mark();
}
c_IntStack::c_IntStack(){
}
c_IntStack* c_IntStack::m_new(Array<int > t_data){
	c_Stack3::m_new2(t_data);
	return this;
}
c_IntStack* c_IntStack::m_new2(){
	c_Stack3::m_new();
	return this;
}
void c_IntStack::mark(){
	c_Stack3::mark();
}
c_BackwardsStack::c_BackwardsStack(){
	m_stack=0;
}
c_BackwardsStack* c_BackwardsStack::m_new(c_Stack3* t_stack){
	this->m_stack=t_stack;
	return this;
}
c_BackwardsStack* c_BackwardsStack::m_new2(){
	return this;
}
c_BackwardsEnumerator* c_BackwardsStack::p_ObjectEnumerator(){
	return (new c_BackwardsEnumerator)->m_new(m_stack);
}
void c_BackwardsStack::mark(){
	Object::mark();
}
c_BackwardsEnumerator::c_BackwardsEnumerator(){
	m_stack=0;
	m_index=0;
}
c_BackwardsEnumerator* c_BackwardsEnumerator::m_new(c_Stack3* t_stack){
	this->m_stack=t_stack;
	m_index=t_stack->m_length;
	return this;
}
c_BackwardsEnumerator* c_BackwardsEnumerator::m_new2(){
	return this;
}
bool c_BackwardsEnumerator::p_HasNext(){
	return m_index>0;
}
int c_BackwardsEnumerator::p_NextObject(){
	m_index-=1;
	return m_stack->m_data[m_index];
}
void c_BackwardsEnumerator::mark(){
	Object::mark();
}
c_DocDoccer::c_DocDoccer(){
	m_maker=0;
}
c_DocDoccer* c_DocDoccer::m_new(c_Makedocs* t_pMakedocs){
	m_maker=t_pMakedocs;
	return this;
}
c_DocDoccer* c_DocDoccer::m_new2(){
	return this;
}
void c_DocDoccer::p_DocIn(String t_pDirectory,c_DocDecl* t_pScope,String t_pDocPath,bool t_p3rdParty){
	String t_docpath=t_pDocPath;
	if((t_docpath).Length()!=0){
		t_docpath=t_docpath+String(L"/",1);
	}
	Array<String > t_=bb_os_LoadDir(t_pDirectory,false,false);
	int t_2=0;
	while(t_2<t_.Length()){
		String t_file=t_[t_2];
		t_2=t_2+1;
		String t_path=t_pDirectory+String(L"/",1)+t_file;
		int t_1=FileType(t_path);
		if(t_1==2){
			if(t_file.EndsWith(String(L".data",5))){
				String t_name=bb_os_StripExt(t_file);
				String t_dst=t_docpath+t_name;
				if(t_p3rdParty){
					int t_p=t_dst.Find(String(L"/",1),0);
					t_dst=t_dst.Slice(t_p+1);
				}
				m_maker->p_CopyDir(t_path,String(L"docs/html/data/",15)+t_dst,true);
			}else{
				c_DocDecl* t_decl=t_pScope->p_GetChild2(t_file);
				if(!((t_decl)!=0)){
					t_decl=(new c_DocDecl)->m_new(903,t_file);
					t_decl->p_Add((new c_DocDecl)->m_new(201,t_docpath));
					t_pScope->p_Add(t_decl);
				}
				p_DocIn(t_path,t_decl,t_docpath+t_file,t_p3rdParty);
			}
		}else{
			if(t_1==1){
				String t_22=bb_os_ExtractExt(t_file);
				if(t_22==String(L"cerberusdoc",11) || t_22==String(L"monkeydoc",9)){
					String t_name2=bb_os_StripExt(t_file);
					c_DocDecl* t_decl2=t_pScope->p_GetChild2(t_name2);
					bool t_newdoc=false;
					if((t_decl2)!=0){
						t_decl2->m_kind=902;
					}else{
						t_decl2=(new c_DocDecl)->m_new(902,t_name2);
						t_newdoc=true;
						t_decl2->p_Add((new c_DocDecl)->m_new(201,t_docpath));
					}
					String t_txt=LoadString(t_path);
					t_txt=bb_stringutil_UnifyLineEndings(t_txt);
					t_decl2->p_Add((new c_DocDecl)->m_new(805,t_txt));
					if(t_newdoc){
						t_pScope->p_Add(t_decl2);
					}
					if(t_p3rdParty){
						t_path=t_pDirectory+String(L"/",1)+t_name2+String(L".png",4);
						if(FileType(t_path)==1){
							String t_filename=String(L"3rd Party Docs_",15)+t_name2+String(L".png",4);
							t_decl2->p_Add((new c_DocDecl)->m_new(211,t_filename));
							m_maker->p_CopyFile(t_path,String(L"docs/html/",10)+t_filename);
						}
					}
				}
			}
		}
	}
}
void c_DocDoccer::p_Doc(){
	p_DocIn(m_maker->m_docpath,m_maker->m_rootdocs,String(),false);
}
void c_DocDoccer::p_Doc3rdParty(){
	c_DocDeclStack* t_decls=m_maker->m_branch3rdparty->p_GetChilds2(210,false);
	if((t_decls)!=0){
		c_Enumerator2* t_=t_decls->p_ObjectEnumerator();
		while(t_->p_HasNext()){
			c_DocDecl* t_d=t_->p_NextObject();
			p_DocIn(t_d->m_ident,m_maker->m_branch3rdparty,String(L"3rd Party Docs",14),true);
		}
	}
}
void c_DocDoccer::mark(){
	Object::mark();
}
c_Map2::c_Map2(){
	m_root=0;
}
c_Map2* c_Map2::m_new(){
	return this;
}
c_Node3* c_Map2::p_FindNode(String t_key){
	c_Node3* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare2(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
c_DocDecl* c_Map2::p_Get2(String t_key){
	c_Node3* t_node=p_FindNode(t_key);
	if((t_node)!=0){
		return t_node->m_value;
	}
	return 0;
}
int c_Map2::p_RotateLeft2(c_Node3* t_node){
	c_Node3* t_child=t_node->m_right;
	t_node->m_right=t_child->m_left;
	if((t_child->m_left)!=0){
		t_child->m_left->m_parent=t_node;
	}
	t_child->m_parent=t_node->m_parent;
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			t_node->m_parent->m_left=t_child;
		}else{
			t_node->m_parent->m_right=t_child;
		}
	}else{
		m_root=t_child;
	}
	t_child->m_left=t_node;
	t_node->m_parent=t_child;
	return 0;
}
int c_Map2::p_RotateRight2(c_Node3* t_node){
	c_Node3* t_child=t_node->m_left;
	t_node->m_left=t_child->m_right;
	if((t_child->m_right)!=0){
		t_child->m_right->m_parent=t_node;
	}
	t_child->m_parent=t_node->m_parent;
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			t_node->m_parent->m_right=t_child;
		}else{
			t_node->m_parent->m_left=t_child;
		}
	}else{
		m_root=t_child;
	}
	t_child->m_right=t_node;
	t_node->m_parent=t_child;
	return 0;
}
int c_Map2::p_InsertFixup2(c_Node3* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node3* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft2(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight2(t_node->m_parent->m_parent);
			}
		}else{
			c_Node3* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight2(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft2(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map2::p_Add3(String t_key,c_DocDecl* t_value){
	c_Node3* t_node=m_root;
	c_Node3* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare2(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return false;
			}
		}
	}
	t_node=(new c_Node3)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			t_parent->m_right=t_node;
		}else{
			t_parent->m_left=t_node;
		}
		p_InsertFixup2(t_node);
	}else{
		m_root=t_node;
	}
	return true;
}
c_Node3* c_Map2::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node3* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator3* c_Map2::p_ObjectEnumerator(){
	return (new c_NodeEnumerator3)->m_new(p_FirstNode());
}
void c_Map2::mark(){
	Object::mark();
}
c_StringMap2::c_StringMap2(){
}
c_StringMap2* c_StringMap2::m_new(){
	c_Map2::m_new();
	return this;
}
int c_StringMap2::p_Compare2(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap2::mark(){
	c_Map2::mark();
}
c_Node3::c_Node3(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node3* c_Node3::m_new(String t_key,c_DocDecl* t_value,int t_color,c_Node3* t_parent){
	this->m_key=t_key;
	this->m_value=t_value;
	this->m_color=t_color;
	this->m_parent=t_parent;
	return this;
}
c_Node3* c_Node3::m_new2(){
	return this;
}
c_Node3* c_Node3::p_NextNode(){
	c_Node3* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node3* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
c_DocDecl* c_Node3::p_Value(){
	return m_value;
}
void c_Node3::mark(){
	Object::mark();
}
c_Map3::c_Map3(){
	m_root=0;
}
c_Map3* c_Map3::m_new(){
	return this;
}
int c_Map3::p_RotateLeft3(c_Node4* t_node){
	c_Node4* t_child=t_node->m_right;
	t_node->m_right=t_child->m_left;
	if((t_child->m_left)!=0){
		t_child->m_left->m_parent=t_node;
	}
	t_child->m_parent=t_node->m_parent;
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			t_node->m_parent->m_left=t_child;
		}else{
			t_node->m_parent->m_right=t_child;
		}
	}else{
		m_root=t_child;
	}
	t_child->m_left=t_node;
	t_node->m_parent=t_child;
	return 0;
}
int c_Map3::p_RotateRight3(c_Node4* t_node){
	c_Node4* t_child=t_node->m_left;
	t_node->m_left=t_child->m_right;
	if((t_child->m_right)!=0){
		t_child->m_right->m_parent=t_node;
	}
	t_child->m_parent=t_node->m_parent;
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			t_node->m_parent->m_right=t_child;
		}else{
			t_node->m_parent->m_left=t_child;
		}
	}else{
		m_root=t_child;
	}
	t_child->m_right=t_node;
	t_node->m_parent=t_child;
	return 0;
}
int c_Map3::p_InsertFixup3(c_Node4* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node4* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft3(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight3(t_node->m_parent->m_parent);
			}
		}else{
			c_Node4* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight3(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft3(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map3::p_Add4(String t_key,c_StringMap2* t_value){
	c_Node4* t_node=m_root;
	c_Node4* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare2(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return false;
			}
		}
	}
	t_node=(new c_Node4)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			t_parent->m_right=t_node;
		}else{
			t_parent->m_left=t_node;
		}
		p_InsertFixup3(t_node);
	}else{
		m_root=t_node;
	}
	return true;
}
c_Node4* c_Map3::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node4* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator2* c_Map3::p_ObjectEnumerator(){
	return (new c_NodeEnumerator2)->m_new(p_FirstNode());
}
void c_Map3::mark(){
	Object::mark();
}
c_StringMap3::c_StringMap3(){
}
c_StringMap3* c_StringMap3::m_new(){
	c_Map3::m_new();
	return this;
}
int c_StringMap3::p_Compare2(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap3::mark(){
	c_Map3::mark();
}
c_Node4::c_Node4(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=0;
	m_color=0;
	m_parent=0;
}
c_Node4* c_Node4::m_new(String t_key,c_StringMap2* t_value,int t_color,c_Node4* t_parent){
	this->m_key=t_key;
	this->m_value=t_value;
	this->m_color=t_color;
	this->m_parent=t_parent;
	return this;
}
c_Node4* c_Node4::m_new2(){
	return this;
}
c_Node4* c_Node4::p_NextNode(){
	c_Node4* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node4* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
String c_Node4::p_Key(){
	return m_key;
}
c_StringMap2* c_Node4::p_Value(){
	return m_value;
}
void c_Node4::mark(){
	Object::mark();
}
c_Map4::c_Map4(){
	m_root=0;
}
c_Map4* c_Map4::m_new(){
	return this;
}
c_Node5* c_Map4::p_FindNode(String t_key){
	c_Node5* t_node=m_root;
	while((t_node)!=0){
		int t_cmp=p_Compare2(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return t_node;
			}
		}
	}
	return t_node;
}
bool c_Map4::p_Contains(String t_key){
	return p_FindNode(t_key)!=0;
}
int c_Map4::p_RotateLeft4(c_Node5* t_node){
	c_Node5* t_child=t_node->m_right;
	t_node->m_right=t_child->m_left;
	if((t_child->m_left)!=0){
		t_child->m_left->m_parent=t_node;
	}
	t_child->m_parent=t_node->m_parent;
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_left){
			t_node->m_parent->m_left=t_child;
		}else{
			t_node->m_parent->m_right=t_child;
		}
	}else{
		m_root=t_child;
	}
	t_child->m_left=t_node;
	t_node->m_parent=t_child;
	return 0;
}
int c_Map4::p_RotateRight4(c_Node5* t_node){
	c_Node5* t_child=t_node->m_left;
	t_node->m_left=t_child->m_right;
	if((t_child->m_right)!=0){
		t_child->m_right->m_parent=t_node;
	}
	t_child->m_parent=t_node->m_parent;
	if((t_node->m_parent)!=0){
		if(t_node==t_node->m_parent->m_right){
			t_node->m_parent->m_right=t_child;
		}else{
			t_node->m_parent->m_left=t_child;
		}
	}else{
		m_root=t_child;
	}
	t_child->m_right=t_node;
	t_node->m_parent=t_child;
	return 0;
}
int c_Map4::p_InsertFixup4(c_Node5* t_node){
	while(((t_node->m_parent)!=0) && t_node->m_parent->m_color==-1 && ((t_node->m_parent->m_parent)!=0)){
		if(t_node->m_parent==t_node->m_parent->m_parent->m_left){
			c_Node5* t_uncle=t_node->m_parent->m_parent->m_right;
			if(((t_uncle)!=0) && t_uncle->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle->m_color=1;
				t_uncle->m_parent->m_color=-1;
				t_node=t_uncle->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_right){
					t_node=t_node->m_parent;
					p_RotateLeft4(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateRight4(t_node->m_parent->m_parent);
			}
		}else{
			c_Node5* t_uncle2=t_node->m_parent->m_parent->m_left;
			if(((t_uncle2)!=0) && t_uncle2->m_color==-1){
				t_node->m_parent->m_color=1;
				t_uncle2->m_color=1;
				t_uncle2->m_parent->m_color=-1;
				t_node=t_uncle2->m_parent;
			}else{
				if(t_node==t_node->m_parent->m_left){
					t_node=t_node->m_parent;
					p_RotateRight4(t_node);
				}
				t_node->m_parent->m_color=1;
				t_node->m_parent->m_parent->m_color=-1;
				p_RotateLeft4(t_node->m_parent->m_parent);
			}
		}
	}
	m_root->m_color=1;
	return 0;
}
bool c_Map4::p_Add5(String t_key,String t_value){
	c_Node5* t_node=m_root;
	c_Node5* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare2(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				return false;
			}
		}
	}
	t_node=(new c_Node5)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			t_parent->m_right=t_node;
		}else{
			t_parent->m_left=t_node;
		}
		p_InsertFixup4(t_node);
	}else{
		m_root=t_node;
	}
	return true;
}
bool c_Map4::p_Set2(String t_key,String t_value){
	c_Node5* t_node=m_root;
	c_Node5* t_parent=0;
	int t_cmp=0;
	while((t_node)!=0){
		t_parent=t_node;
		t_cmp=p_Compare2(t_key,t_node->m_key);
		if(t_cmp>0){
			t_node=t_node->m_right;
		}else{
			if(t_cmp<0){
				t_node=t_node->m_left;
			}else{
				t_node->m_value=t_value;
				return false;
			}
		}
	}
	t_node=(new c_Node5)->m_new(t_key,t_value,-1,t_parent);
	if((t_parent)!=0){
		if(t_cmp>0){
			t_parent->m_right=t_node;
		}else{
			t_parent->m_left=t_node;
		}
		p_InsertFixup4(t_node);
	}else{
		m_root=t_node;
	}
	return true;
}
c_Node5* c_Map4::p_FirstNode(){
	if(!((m_root)!=0)){
		return 0;
	}
	c_Node5* t_node=m_root;
	while((t_node->m_left)!=0){
		t_node=t_node->m_left;
	}
	return t_node;
}
c_NodeEnumerator* c_Map4::p_ObjectEnumerator(){
	return (new c_NodeEnumerator)->m_new(p_FirstNode());
}
void c_Map4::mark(){
	Object::mark();
}
c_StringMap4::c_StringMap4(){
}
c_StringMap4* c_StringMap4::m_new(){
	c_Map4::m_new();
	return this;
}
int c_StringMap4::p_Compare2(String t_lhs,String t_rhs){
	return t_lhs.Compare(t_rhs);
}
void c_StringMap4::mark(){
	c_Map4::mark();
}
c_Node5::c_Node5(){
	m_key=String();
	m_right=0;
	m_left=0;
	m_value=String();
	m_color=0;
	m_parent=0;
}
c_Node5* c_Node5::m_new(String t_key,String t_value,int t_color,c_Node5* t_parent){
	this->m_key=t_key;
	this->m_value=t_value;
	this->m_color=t_color;
	this->m_parent=t_parent;
	return this;
}
c_Node5* c_Node5::m_new2(){
	return this;
}
c_Node5* c_Node5::p_NextNode(){
	c_Node5* t_node=0;
	if((m_right)!=0){
		t_node=m_right;
		while((t_node->m_left)!=0){
			t_node=t_node->m_left;
		}
		return t_node;
	}
	t_node=this;
	c_Node5* t_parent=this->m_parent;
	while(((t_parent)!=0) && t_node==t_parent->m_right){
		t_node=t_parent;
		t_parent=t_parent->m_parent;
	}
	return t_parent;
}
String c_Node5::p_Key(){
	return m_key;
}
String c_Node5::p_Value(){
	return m_value;
}
void c_Node5::mark(){
	Object::mark();
}
c_NodeEnumerator::c_NodeEnumerator(){
	m_node=0;
}
c_NodeEnumerator* c_NodeEnumerator::m_new(c_Node5* t_node){
	this->m_node=t_node;
	return this;
}
c_NodeEnumerator* c_NodeEnumerator::m_new2(){
	return this;
}
bool c_NodeEnumerator::p_HasNext(){
	return m_node!=0;
}
c_Node5* c_NodeEnumerator::p_NextObject(){
	c_Node5* t_t=m_node;
	m_node=m_node->p_NextNode();
	return t_t;
}
void c_NodeEnumerator::mark(){
	Object::mark();
}
c_PageMaker::c_PageMaker(){
	m__template=String();
	m__decls=(new c_StringMap)->m_new();
	m__scopes=(new c_Stack4)->m_new();
	m__lists=(new c_Stack5)->m_new();
	m__iters=(new c_IntStack)->m_new2();
}
c_PageMaker* c_PageMaker::m_new(String t_template){
	m__template=t_template;
	m__scopes->p_Push10(m__decls);
	return this;
}
c_PageMaker* c_PageMaker::m_new2(){
	return this;
}
void c_PageMaker::p_Clear(){
	m__decls->p_Clear();
	m__scopes->p_Clear();
	m__scopes->p_Push10(m__decls);
	m__lists->p_Clear();
}
void c_PageMaker::p_SetString(String t_name,String t_value){
	m__scopes->p_Top()->p_Set(t_name,((new c_StringObject)->m_new3(t_value)));
}
void c_PageMaker::p_BeginList(String t_name){
	c_Stack4* t_list=(new c_Stack4)->m_new();
	m__scopes->p_Top()->p_Set(t_name,(t_list));
	m__scopes->p_Push10(0);
	m__lists->p_Push13(t_list);
}
void c_PageMaker::p_AddItem(){
	c_StringMap* t_scope=(new c_StringMap)->m_new();
	m__scopes->p_Pop();
	m__scopes->p_Push10(t_scope);
	m__lists->p_Top()->p_Push10(t_scope);
}
void c_PageMaker::p_EndList(){
	m__scopes->p_Pop();
	m__lists->p_Pop();
}
Object* c_PageMaker::p_GetValue(String t_name){
	for(int t_i=m__scopes->p_Length2()-1;t_i>=0;t_i=t_i+-1){
		c_StringMap* t_sc=m__scopes->p_Get(t_i);
		if(t_sc->p_Contains(t_name)){
			return t_sc->p_Get2(t_name);
		}
	}
	return 0;
}
String c_PageMaker::p_GetString(String t_name){
	Object* t_val=p_GetValue(t_name);
	c_StringObject* t_str=dynamic_cast<c_StringObject*>(t_val);
	if((t_str)!=0){
		return t_str->m_value;
	}
	c_Stack4* t_list=dynamic_cast<c_Stack4*>(t_val);
	if(((t_list)!=0) && ((t_list->p_Length2())!=0)){
		return String(t_list->p_Length2());
	}
	return String();
}
c_Stack4* c_PageMaker::p_GetList(String t_name){
	Object* t_val=p_GetValue(t_name);
	c_Stack4* t_list=dynamic_cast<c_Stack4*>(t_val);
	if(((t_list)!=0) && ((t_list->p_Length2())!=0)){
		return t_list;
	}
	return 0;
}
String c_PageMaker::p_MakePage(){
	m__iters->p_Clear();
	m__lists->p_Clear();
	m__scopes->p_Clear();
	m__scopes->p_Push10(m__decls);
	c_StringStack* t_output=(new c_StringStack)->m_new2();
	int t_i=0;
	int t_ifnest=0;
	int t_iftrue=0;
	do{
		int t_i0=m__template.Find(String(L"${",2),t_i);
		if(t_i0==-1){
			break;
		}
		int t_i1=m__template.Find(String(L"}",1),t_i0+2);
		if(t_i1==-1){
			break;
		}
		bool t_cc=t_ifnest==t_iftrue;
		if(t_cc && t_i<t_i0){
			t_output->p_Push(m__template.Slice(t_i,t_i0));
		}
		t_i=t_i1+1;
		Array<String > t_bits=m__template.Slice(t_i0+2,t_i1).Split(String(L" ",1)).Resize(5);
		String t_1=t_bits[0];
		if(t_1==String(L"IF",2)){
			t_ifnest+=1;
			if(t_cc){
				int t_i2=1;
				bool t_inv=false;
				if(t_bits[t_i2]==String(L"NOT",3)){
					t_inv=true;
					t_i2+=1;
				}
				if(t_bits[t_i2]==String(L"FIRST",5)){
					t_cc=m__iters->p_Get(m__iters->p_Length2()-2)==0;
				}else{
					if(t_bits[t_i2]==String(L"LAST",4)){
						t_cc=m__iters->p_Get(m__iters->p_Length2()-2)==m__lists->p_Top()->p_Length2()-1;
					}else{
						if(t_bits[t_i2+1]==String(L"EQ",2)){
							t_cc=p_GetString(t_bits[t_i2])==t_bits[t_i2+2];
						}else{
							if(t_bits[t_i2+1]==String(L"NE",2)){
								t_cc=p_GetString(t_bits[t_i2])!=t_bits[t_i2+2];
							}else{
								t_cc=p_GetString(t_bits[t_i2])!=String();
							}
						}
					}
				}
				if(t_inv){
					t_cc=!t_cc;
				}
				if(t_cc){
					t_iftrue=t_ifnest;
				}
			}
		}else{
			if(t_1==String(L"ENDIF",5)){
				t_ifnest-=1;
				t_iftrue=bb_math_Min(t_iftrue,t_ifnest);
			}else{
				if(t_1==String(L"FOR",3)){
					t_ifnest+=1;
					if(t_cc){
						c_Stack4* t_list=p_GetList(t_bits[1]);
						if((t_list)!=0){
							t_iftrue=t_ifnest;
							m__iters->p_Push7(0);
							m__iters->p_Push7(t_i);
							m__lists->p_Push13(t_list);
							m__scopes->p_Push10(t_list->p_Get(0));
						}
					}
				}else{
					if(t_1==String(L"NEXT",4)){
						if(t_cc){
							m__scopes->p_Pop();
							c_Stack4* t_list2=m__lists->p_Top();
							int t_p=m__iters->p_Pop();
							int t_j=m__iters->p_Pop()+1;
							if(t_j<t_list2->p_Length2()){
								m__iters->p_Push7(t_j);
								m__iters->p_Push7(t_p);
								m__scopes->p_Push10(t_list2->p_Get(t_j));
								t_i=t_p;
							}else{
								m__lists->p_Pop();
							}
						}
						t_ifnest-=1;
						t_iftrue=bb_math_Min(t_iftrue,t_ifnest);
					}else{
						if(t_cc){
							String t_str=p_GetString(t_bits[0]);
							if((t_str).Length()!=0){
								t_output->p_Push(t_str);
							}
						}
					}
				}
			}
		}
	}while(!(false));
	if(t_i<m__template.Length()){
		t_output->p_Push(m__template.Slice(t_i));
	}
	return t_output->p_Join(String());
}
void c_PageMaker::mark(){
	Object::mark();
}
c_Stack4::c_Stack4(){
	m_data=Array<c_StringMap* >();
	m_length=0;
}
c_Stack4* c_Stack4::m_new(){
	return this;
}
c_Stack4* c_Stack4::m_new2(Array<c_StringMap* > t_data){
	this->m_data=t_data.Slice(0);
	this->m_length=t_data.Length();
	return this;
}
void c_Stack4::p_Push10(c_StringMap* t_value){
	if(m_length==m_data.Length()){
		m_data=m_data.Resize(m_length*2+10);
	}
	m_data[m_length]=t_value;
	m_length+=1;
}
void c_Stack4::p_Push11(Array<c_StringMap* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push10(t_values[t_offset+t_i]);
	}
}
void c_Stack4::p_Push12(Array<c_StringMap* > t_values,int t_offset){
	p_Push11(t_values,t_offset,t_values.Length()-t_offset);
}
c_StringMap* c_Stack4::m_NIL;
void c_Stack4::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		m_data[t_i]=m_NIL;
	}
	m_length=0;
}
c_StringMap* c_Stack4::p_Top(){
	return m_data[m_length-1];
}
c_StringMap* c_Stack4::p_Pop(){
	m_length-=1;
	c_StringMap* t_v=m_data[m_length];
	m_data[m_length]=m_NIL;
	return t_v;
}
void c_Stack4::p_Length(int t_newlength){
	if(t_newlength<m_length){
		for(int t_i=t_newlength;t_i<m_length;t_i=t_i+1){
			m_data[t_i]=m_NIL;
		}
	}else{
		if(t_newlength>m_data.Length()){
			m_data=m_data.Resize(bb_math_Max(m_length*2+10,t_newlength));
		}
	}
	m_length=t_newlength;
}
int c_Stack4::p_Length2(){
	return m_length;
}
c_StringMap* c_Stack4::p_Get(int t_index){
	return m_data[t_index];
}
void c_Stack4::mark(){
	Object::mark();
}
c_Markdown::c_Markdown(){
	m__resolver=0;
	m__prettifier=0;
	m__blk=String();
}
c_Markdown* c_Markdown::m_new(c_ILinkResolver* t_resolver,c_IPrettifier* t_prettifier){
	m__resolver=t_resolver;
	m__prettifier=t_prettifier;
	return this;
}
c_Markdown* c_Markdown::m_new2(){
	return this;
}
String c_Markdown::p_Prettify(String t_text){
	if((m__prettifier)!=0){
		return m__prettifier->p_PrettifyLine(t_text);
	}
	return t_text;
}
String c_Markdown::p_SetBlock(String t_blk){
	String t_t=String();
	if(m__blk!=t_blk){
		if(m__blk==String(L"pre",3) && ((m__prettifier)!=0)){
			t_t=m__prettifier->p_EndPrettyBlock();
		}else{
			if((m__blk).Length()!=0){
				t_t=String(L"</",2)+m__blk+String(L">",1);
				if((t_blk).Length()!=0){
					t_t=t_t+String(L"\n",1);
				}
			}
		}
		m__blk=t_blk;
		if(m__blk==String(L"pre",3) && ((m__prettifier)!=0)){
			t_t=t_t+m__prettifier->p_BeginPrettyBlock();
		}else{
			if((m__blk).Length()!=0){
				t_t=t_t+(String(L"<",1)+m__blk+String(L">",1));
				t_t=t_t+String(L"\n",1);
			}
		}
	}
	return t_t;
}
int c_Markdown::p_Find3(String t_src,String t_text,int t_start){
	do{
		int t_i=t_src.Find(t_text,t_start);
		if(t_i==-1){
			return -1;
		}
		int t_j=t_i;
		while(t_j>0 && (int)t_src[t_j-1]==92){
			t_j-=1;
		}
		if(t_j!=t_i && (t_i-t_j&1)==1){
			t_start=t_i+t_text.Length();
		}else{
			return t_i;
		}
	}while(!(false));
}
String c_Markdown::p_ReplaceSpanTags(String t_src,String t_tag,String t_html){
	int t_i=0;
	int t_l=t_tag.Length();
	do{
		int t_i0=p_Find3(t_src,t_tag,t_i);
		if(t_i0==-1){
			break;
		}
		int t_i1=p_Find3(t_src,t_tag,t_i0+t_l);
		if(t_i1==-1 || t_i1==t_i0+t_l){
			break;
		}
		String t_r=String(L"<",1)+t_html+String(L">",1)+t_src.Slice(t_i0+t_l,t_i1)+String(L"</",2)+t_html+String(L">",1);
		t_src=t_src.Slice(0,t_i0)+t_r+t_src.Slice(t_i1+t_l);
		t_i=t_i0+t_r.Length();
	}while(!(false));
	return t_src;
}
String c_Markdown::p_ReplacePrefixTags(String t_src,String t_tag,String t_html){
	int t_i=0;
	int t_l=t_tag.Length();
	do{
		int t_i0=p_Find3(t_src,t_tag,t_i);
		if(t_i0==-1){
			break;
		}
		int t_i1=t_i0+t_l;
		while(t_i1<t_src.Length()){
			int t_c=(int)t_src[t_i1];
			if(t_c==95 || t_c>=65 && t_c<=90 || t_c>=97 && t_c<=122 || t_i1>t_i0+t_l && t_c>=48 && t_c<=57){
				t_i1+=1;
				continue;
			}
			break;
		}
		if(t_i1==t_i0+t_l){
			t_i+=t_l;
			continue;
		}
		String t_r=String(L"<",1)+t_html+String(L">",1)+t_src.Slice(t_i0+t_l,t_i1)+String(L"</",2)+t_html+String(L">",1);
		t_src=t_src.Slice(0,t_i0)+t_r+t_src.Slice(t_i1);
		t_i=t_i0+t_r.Length();
	}while(!(false));
	return t_src;
}
String c_Markdown::p_ReplaceLinks(String t_src){
	int t_i=0;
	do{
		int t_i0=p_Find3(t_src,String(L"[[",2),t_i);
		if(t_i0==-1){
			break;
		}
		int t_i1=p_Find3(t_src,String(L"]]",2),t_i0+2);
		if(t_i1==-1){
			break;
		}
		String t_p=t_src.Slice(t_i0+2,t_i1);
		String t_t=String();
		int t_j=t_p.Find(String(L"|",1),0);
		if(t_j!=-1){
			t_t=t_p.Slice(t_j+1);
			t_p=t_p.Slice(0,t_j);
		}
		String t_r=m__resolver->p_ResolveLink(t_p,t_t);
		t_src=t_src.Slice(0,t_i0)+t_r+t_src.Slice(t_i1+2);
		t_i=t_i0+t_r.Length();
	}while(!(false));
	t_i=0;
	do{
		int t_i02=p_Find3(t_src,String(L"![",2),t_i);
		if(t_i02==-1){
			break;
		}
		int t_i12=p_Find3(t_src,String(L"](",2),t_i02+1);
		if(t_i12==-1 || t_i12==t_i02+1){
			break;
		}
		int t_i2=p_Find3(t_src,String(L")",1),t_i12+2);
		if(t_i2==-1 || t_i2==t_i12+2){
			break;
		}
		String t_t2=t_src.Slice(t_i02+2,t_i12);
		String t_p2=t_src.Slice(t_i12+2,t_i2);
		String t_r2=m__resolver->p_ResolveLink(String(L"<img>",5)+t_p2,t_t2);
		t_src=t_src.Slice(0,t_i02)+t_r2+t_src.Slice(t_i2+1);
		t_i=t_i02+t_r2.Length();
	}while(!(false));
	t_i=0;
	do{
		int t_i03=p_Find3(t_src,String(L"[",1),t_i);
		if(t_i03==-1){
			break;
		}
		int t_i13=p_Find3(t_src,String(L"](",2),t_i03+1);
		if(t_i13==-1 || t_i13==t_i03+1){
			break;
		}
		int t_i22=p_Find3(t_src,String(L")",1),t_i13+2);
		if(t_i22==-1 || t_i22==t_i13+2){
			break;
		}
		String t_t3=t_src.Slice(t_i03+1,t_i13);
		String t_p3=t_src.Slice(t_i13+2,t_i22);
		String t_r3=m__resolver->p_ResolveLink(t_p3,t_t3);
		t_src=t_src.Slice(0,t_i03)+t_r3+t_src.Slice(t_i22+1);
		t_i=t_i03+t_r3.Length();
	}while(!(false));
	return t_src;
}
String c_Markdown::p_ReplaceEscs(String t_src){
	int t_i=0;
	do{
		int t_i0=t_src.Find(String(L"\\",1),t_i);
		if(t_i0==-1){
			break;
		}
		String t_r=t_src.Slice(t_i0+1,t_i0+2);
		String t_1=t_r;
		if(t_1==String(L"<",1)){
			t_r=String(L"&lt;",4);
		}else{
			if(t_1==String(L">",1)){
				t_r=String(L"&gt;",4);
			}else{
				if(t_1==String(L"&",1)){
					t_r=String(L"&amp;",5);
				}
			}
		}
		t_src=t_src.Slice(0,t_i0)+t_r+t_src.Slice(t_i0+2);
		t_i=t_i0+t_r.Length();
	}while(!(false));
	return t_src;
}
String c_Markdown::p_SpanToHtml(String t_src){
	t_src=p_ReplaceSpanTags(t_src,String(L"`",1),String(L"code",4));
	t_src=p_ReplaceSpanTags(t_src,String(L"*",1),String(L"b",1));
	t_src=p_ReplaceSpanTags(t_src,String(L"%",1),String(L"i",1));
	t_src=p_ReplacePrefixTags(t_src,String(L"@",1),String(L"b",1));
	t_src=p_ReplaceLinks(t_src);
	t_src=p_ReplaceEscs(t_src);
	return t_src;
}
String c_Markdown::p_LineToHtml(String t_src){
	if(m__blk==String(L"pre",3)){
		int t_i=t_src.Find(String(L"</pre>",6),0);
		if(t_i==-1){
			return p_Prettify(t_src);
		}
		if((t_src.Slice(0,t_i).Trim()).Length()!=0){
			return p_Prettify(t_src.Slice(0,t_i))+p_SetBlock(String());
		}
		return p_SetBlock(String());
	}
	if(!((t_src).Length()!=0)){
		if(m__blk==String(L"ol",2) || m__blk==String(L"ul",2)){
			return String(L"<ul></ul>",9);
		}else{
			if(m__blk==String()){
				return String();
			}else{
				return p_SetBlock(String());
			}
		}
	}
	if(t_src==String(L"-",1) || t_src==String(L"--",2) || t_src==String(L"---",3)){
		return p_SetBlock(String())+String(L"<hr>",4);
	}
	if(t_src.StartsWith(String(L"<pre>",5))){
		String t_t=p_SetBlock(String(L"pre",3));
		String t_source=t_src.Slice(5).Trim();
		if(t_source.EndsWith(String(L"</pre>",6))){
			t_source=t_source.Slice(0,-6);
		}
		if((t_source).Length()!=0){
			return t_t+p_Prettify(t_source);
		}
		return t_t;
	}
	if(t_src.StartsWith(String(L"| ",2))){
		t_src=p_SpanToHtml(t_src);
		c_StringStack* t_bits=(new c_StringStack)->m_new2();
		int t_i2=1;
		do{
			int t_i0=p_Find3(t_src,String(L"|",1),t_i2);
			if(t_i0==-1){
				break;
			}
			t_bits->p_Push(t_src.Slice(t_i2,t_i0).Trim());
			t_i2=t_i0+1;
		}while(!(false));
		t_bits->p_Push(t_src.Slice(t_i2).Trim());
		String t_tag=String(L"td",2);
		if(m__blk!=String(L"table",5)){
			t_tag=String(L"th",2);
		}
		return p_SetBlock(String(L"table",5))+String(L"<tr><",5)+t_tag+String(L">",1)+t_bits->p_Join(String(L"</",2)+t_tag+String(L"><",2)+t_tag+String(L">",1))+String(L"</",2)+t_tag+String(L"></tr>",6);
	}
	if(t_src.StartsWith(String(L">",1))){
		int t_i3=1;
		while(t_i3<t_src.Length() && (int)t_src[t_i3]==62){
			t_i3+=1;
		}
		if(t_i3<t_src.Length() && (int)t_src[t_i3]<=32){
			String t_t2=p_SetBlock(String());
			t_src=p_SpanToHtml(t_src.Slice(t_i3+1));
			return t_t2+String(L"<h",2)+String(t_i3)+String(L">",1)+t_src+String(L"</h",3)+String(t_i3)+String(L">",1);
		}
	}
	if(t_src.StartsWith(String(L"* ",2))){
		String t_t3=p_SetBlock(String(L"ul",2));
		return t_t3+String(L"<li>",4)+p_SpanToHtml(t_src.Slice(2))+String(L"</li>",5);
	}
	if(t_src.StartsWith(String(L"+ ",2))){
		String t_t4=p_SetBlock(String(L"ol",2));
		return t_t4+String(L"<li>",4)+p_SpanToHtml(t_src.Slice(2))+String(L"</li>",5);
	}
	String t_t5=p_SetBlock(String(L"p",1));
	int t_i4=p_Find3(t_src,String(L"~n",2),t_src.Length()-2);
	if(t_i4!=-1){
		t_src=t_src.Slice(0,-2)+String(L"<br>",4);
	}
	t_src=p_SpanToHtml(t_src);
	return t_t5+t_src;
}
String c_Markdown::p_ToHtml(String t_src){
	String t_html=String();
	if(t_src.Contains(String(L"\n",1))){
		c_StringStack* t_buf=(new c_StringStack)->m_new2();
		Array<String > t_=t_src.Split(String(L"\n",1));
		int t_2=0;
		while(t_2<t_.Length()){
			String t_line=t_[t_2];
			t_2=t_2+1;
			t_html=p_LineToHtml(t_line);
			if((t_html).Length()!=0){
				t_buf->p_Push(t_html);
			}
		}
		t_html=t_buf->p_Join(String(L"\n",1));
	}else{
		t_html=p_LineToHtml(t_src);
	}
	if((m__blk).Length()!=0){
		return t_html+String(L"\n",1)+p_SetBlock(String());
	}
	return t_html;
}
void c_Markdown::mark(){
	Object::mark();
}
c_Stack5::c_Stack5(){
	m_data=Array<c_Stack4* >();
	m_length=0;
}
c_Stack5* c_Stack5::m_new(){
	return this;
}
c_Stack5* c_Stack5::m_new2(Array<c_Stack4* > t_data){
	this->m_data=t_data.Slice(0);
	this->m_length=t_data.Length();
	return this;
}
c_Stack4* c_Stack5::m_NIL;
void c_Stack5::p_Clear(){
	for(int t_i=0;t_i<m_length;t_i=t_i+1){
		m_data[t_i]=m_NIL;
	}
	m_length=0;
}
void c_Stack5::p_Push13(c_Stack4* t_value){
	if(m_length==m_data.Length()){
		m_data=m_data.Resize(m_length*2+10);
	}
	m_data[m_length]=t_value;
	m_length+=1;
}
void c_Stack5::p_Push14(Array<c_Stack4* > t_values,int t_offset,int t_count){
	for(int t_i=0;t_i<t_count;t_i=t_i+1){
		p_Push13(t_values[t_offset+t_i]);
	}
}
void c_Stack5::p_Push15(Array<c_Stack4* > t_values,int t_offset){
	p_Push14(t_values,t_offset,t_values.Length()-t_offset);
}
c_Stack4* c_Stack5::p_Top(){
	return m_data[m_length-1];
}
c_Stack4* c_Stack5::p_Pop(){
	m_length-=1;
	c_Stack4* t_v=m_data[m_length];
	m_data[m_length]=m_NIL;
	return t_v;
}
void c_Stack5::mark(){
	Object::mark();
}
c_StringObject::c_StringObject(){
	m_value=String();
}
c_StringObject* c_StringObject::m_new(int t_value){
	this->m_value=String(t_value);
	return this;
}
c_StringObject* c_StringObject::m_new2(Float t_value){
	this->m_value=String(t_value);
	return this;
}
c_StringObject* c_StringObject::m_new3(String t_value){
	this->m_value=t_value;
	return this;
}
c_StringObject* c_StringObject::m_new4(){
	return this;
}
void c_StringObject::mark(){
	Object::mark();
}
String bb_stringutil_HtmlEscape(String t_str){
	return t_str.Replace(String(L"&",1),String(L"&amp;",5)).Replace(String(L"<",1),String(L"&lt;",4)).Replace(String(L">",1),String(L"&gt;",4));
}
int bb_math_Min(int t_x,int t_y){
	if(t_x<t_y){
		return t_x;
	}
	return t_y;
}
Float bb_math_Min2(Float t_x,Float t_y){
	if(t_x<t_y){
		return t_x;
	}
	return t_y;
}
c_NodeEnumerator2::c_NodeEnumerator2(){
	m_node=0;
}
c_NodeEnumerator2* c_NodeEnumerator2::m_new(c_Node4* t_node){
	this->m_node=t_node;
	return this;
}
c_NodeEnumerator2* c_NodeEnumerator2::m_new2(){
	return this;
}
bool c_NodeEnumerator2::p_HasNext(){
	return m_node!=0;
}
c_Node4* c_NodeEnumerator2::p_NextObject(){
	c_Node4* t_t=m_node;
	m_node=m_node->p_NextNode();
	return t_t;
}
void c_NodeEnumerator2::mark(){
	Object::mark();
}
c_NodeEnumerator3::c_NodeEnumerator3(){
	m_node=0;
}
c_NodeEnumerator3* c_NodeEnumerator3::m_new(c_Node3* t_node){
	this->m_node=t_node;
	return this;
}
c_NodeEnumerator3* c_NodeEnumerator3::m_new2(){
	return this;
}
bool c_NodeEnumerator3::p_HasNext(){
	return m_node!=0;
}
c_Node3* c_NodeEnumerator3::p_NextObject(){
	c_Node3* t_t=m_node;
	m_node=m_node->p_NextNode();
	return t_t;
}
void c_NodeEnumerator3::mark(){
	Object::mark();
}
int bbMain(){
	(new c_Makedocs)->m_new();
	return 0;
}
int bbInit(){
	GC_CTOR
	c_DocDecl::m__uid=0;
	c_Stack2::m_NIL=0;
	c_Toker::m__keywords=0;
	c_Toker::m__symbols=0;
	c_Stack::m_NIL=String();
	c_Stack3::m_NIL=0;
	c_DocDecl::m__root=0;
	c_DocDecl::m__modroot=0;
	c_DocDecl::m__modules=0;
	c_DocDecl::m__docroot=0;
	c_DocDecl::m__documents=0;
	c_DocDecl::m__primnames=(new c_StringStack)->m_new2();
	c_DocDecl::m__primlinks=(new c_StringMap2)->m_new();
	c_DocDecl::m__searchid=0;
	c_Makedocs::m_resolvecode=0;
	c_Stack4::m_NIL=0;
	c_Stack5::m_NIL=0;
	return 0;
}
void gc_mark(){
}
//${TRANSCODE_END}

String BBPathToFilePath( String path ){
	return path;
}

int main( int argc,const char **argv ){

	new BBGame();

	try{
	
		bb_std_main( argc,argv );
		
	}catch( ThrowableObject *ex ){
	
		bbPrint( "Cerberus Runtime Error : Uncaught Cerberus Exception" );
	
	}catch( const char *err ){
	
	}
}

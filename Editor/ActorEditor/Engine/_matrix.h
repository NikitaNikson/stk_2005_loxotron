#ifndef __M__
#define __M__
/*
*	DirectX-compliant, ie row-column order, ie m[Row][Col].
*	Same as:
*	m11  m12  m13  m14	first row.
*	m21  m22  m23  m24	second row.
*	m31  m32  m33  m34	third row.
*	m41  m42  m43  m44	fourth row.
*	Translation is (m41, m42, m43), (m14, m24, m34, m44) = (0, 0, 0, 1).
*	Stored in memory as m11 m12 m13 m14 m21...
*
*	Multiplication rules:
*
*	[x'y'z'1] = [xyz1][M]
*
*	x' = x*m11 + y*m21 + z*m31 + m41
*	y' = x*m12 + y*m22 + z*m32 + m42
*	z' = x*m13 + y*m23 + z*m33 + m43
*	1' =     0 +     0 +     0 + m44
*/

// NOTE_1: positive angle means clockwise rotation
// NOTE_2: mul(A,B) means transformation B, followed by A
// NOTE_3: I,J,K,C equals to R,N,D,T
// NOTE_4: The rotation sequence is ZXY

template <class T>
struct _matrix {
public:
	typedef T			TYPE;
	typedef _matrix<T>	Self;
	typedef Self&		SelfRef;
	typedef const Self&	SelfCRef;
	typedef _vector<T>	Tvector;
public:
	union {
		struct {						// Direct definition
            T _11, _12, _13, _14;
            T _21, _22, _23, _24;
            T _31, _32, _33, _34;
            T _41, _42, _43, _44;
		};
    	struct{
    		Tvector i;	T	_14_;
    		Tvector j;	T	_24_;
    		Tvector k;	T	_34_;
    		Tvector c;	T	_44_;
        };
		T m[4][4];					// Array
	};

	IC	D3DMATRIX	*d3d(void) const { return (D3DMATRIX *)this; };

	// Class members
	IC	void	set			(const Self &a) {
		CopyMemory			(this,&a,16*sizeof(T));
	}
	IC	void	set			(const Tvector& R,const Tvector& N,const Tvector& D,const Tvector& C) {
		i.set(R); _14_=0;
		j.set(N); _24_=0;
		k.set(D); _34_=0;
		c.set(C); _44_=1;
	}
	IC	void	identity	(void) 
	{
		_11=1; _12=0; _13=0; _14=0;
		_21=0; _22=1; _23=0; _24=0;
		_31=0; _32=0; _33=1; _34=0;
		_41=0; _42=0; _43=0; _44=1;
	}
	IC	void	get_rapid	(_matrix33& R) const;
	IC	void	rotation	(const _quaternion &Q);
	IC	void	mk_xform	(const _quaternion &Q, const Tvector &V);
	// Multiply RES = A[4x4]*B[4x4] (WITH projection)
	IC	void	mul			(const Self &A,const Self &B)
	{
		m[0][0] = A.m[0][0] * B.m[0][0] + A.m[1][0] * B.m[0][1] + A.m[2][0] * B.m[0][2] + A.m[3][0] * B.m[0][3];
		m[0][1] = A.m[0][1] * B.m[0][0] + A.m[1][1] * B.m[0][1] + A.m[2][1] * B.m[0][2] + A.m[3][1] * B.m[0][3];
		m[0][2] = A.m[0][2] * B.m[0][0] + A.m[1][2] * B.m[0][1] + A.m[2][2] * B.m[0][2] + A.m[3][2] * B.m[0][3];
		m[0][3] = A.m[0][3] * B.m[0][0] + A.m[1][3] * B.m[0][1] + A.m[2][3] * B.m[0][2] + A.m[3][3] * B.m[0][3];

		m[1][0] = A.m[0][0] * B.m[1][0] + A.m[1][0] * B.m[1][1] + A.m[2][0] * B.m[1][2] + A.m[3][0] * B.m[1][3];
		m[1][1] = A.m[0][1] * B.m[1][0] + A.m[1][1] * B.m[1][1] + A.m[2][1] * B.m[1][2] + A.m[3][1] * B.m[1][3];
		m[1][2] = A.m[0][2] * B.m[1][0] + A.m[1][2] * B.m[1][1] + A.m[2][2] * B.m[1][2] + A.m[3][2] * B.m[1][3];
		m[1][3] = A.m[0][3] * B.m[1][0] + A.m[1][3] * B.m[1][1] + A.m[2][3] * B.m[1][2] + A.m[3][3] * B.m[1][3];

		m[2][0] = A.m[0][0] * B.m[2][0] + A.m[1][0] * B.m[2][1] + A.m[2][0] * B.m[2][2] + A.m[3][0] * B.m[2][3];
		m[2][1] = A.m[0][1] * B.m[2][0] + A.m[1][1] * B.m[2][1] + A.m[2][1] * B.m[2][2] + A.m[3][1] * B.m[2][3];
		m[2][2] = A.m[0][2] * B.m[2][0] + A.m[1][2] * B.m[2][1] + A.m[2][2] * B.m[2][2] + A.m[3][2] * B.m[2][3];
		m[2][3] = A.m[0][3] * B.m[2][0] + A.m[1][3] * B.m[2][1] + A.m[2][3] * B.m[2][2] + A.m[3][3] * B.m[2][3];

		m[3][0] = A.m[0][0] * B.m[3][0] + A.m[1][0] * B.m[3][1] + A.m[2][0] * B.m[3][2] + A.m[3][0] * B.m[3][3];
		m[3][1] = A.m[0][1] * B.m[3][0] + A.m[1][1] * B.m[3][1] + A.m[2][1] * B.m[3][2] + A.m[3][1] * B.m[3][3];
		m[3][2] = A.m[0][2] * B.m[3][0] + A.m[1][2] * B.m[3][1] + A.m[2][2] * B.m[3][2] + A.m[3][2] * B.m[3][3];
		m[3][3] = A.m[0][3] * B.m[3][0] + A.m[1][3] * B.m[3][1] + A.m[2][3] * B.m[3][2] + A.m[3][3] * B.m[3][3];
	}

	// Multiply RES = A[4x3]*B[4x3] (no projection), faster than ordinary multiply
	IC	void	mul_43		(const Self &A,const Self &B)
	{
		m[0][0] = A.m[0][0] * B.m[0][0] + A.m[1][0] * B.m[0][1] + A.m[2][0] * B.m[0][2];
		m[0][1] = A.m[0][1] * B.m[0][0] + A.m[1][1] * B.m[0][1] + A.m[2][1] * B.m[0][2];
		m[0][2] = A.m[0][2] * B.m[0][0] + A.m[1][2] * B.m[0][1] + A.m[2][2] * B.m[0][2];
		m[0][3] = 0;

		m[1][0] = A.m[0][0] * B.m[1][0] + A.m[1][0] * B.m[1][1] + A.m[2][0] * B.m[1][2];
		m[1][1] = A.m[0][1] * B.m[1][0] + A.m[1][1] * B.m[1][1] + A.m[2][1] * B.m[1][2];
		m[1][2] = A.m[0][2] * B.m[1][0] + A.m[1][2] * B.m[1][1] + A.m[2][2] * B.m[1][2];
		m[1][3] = 0;

		m[2][0] = A.m[0][0] * B.m[2][0] + A.m[1][0] * B.m[2][1] + A.m[2][0] * B.m[2][2];
		m[2][1] = A.m[0][1] * B.m[2][0] + A.m[1][1] * B.m[2][1] + A.m[2][1] * B.m[2][2];
		m[2][2] = A.m[0][2] * B.m[2][0] + A.m[1][2] * B.m[2][1] + A.m[2][2] * B.m[2][2];
		m[2][3] = 0;

		m[3][0] = A.m[0][0] * B.m[3][0] + A.m[1][0] * B.m[3][1] + A.m[2][0] * B.m[3][2] + A.m[3][0];
		m[3][1] = A.m[0][1] * B.m[3][0] + A.m[1][1] * B.m[3][1] + A.m[2][1] * B.m[3][2] + A.m[3][1];
		m[3][2] = A.m[0][2] * B.m[3][0] + A.m[1][2] * B.m[3][1] + A.m[2][2] * B.m[3][2] + A.m[3][2];
		m[3][3] = 1;
	}
	IC	void	mulA		( const Self &A )	// mul after 
	{
    	Self B; B.set( *this ); 	mul		( A, B );
    };
	IC	void	mulB		( const Self &B )	// mul before
	{
		Self A; A.set( *this ); 	mul		( A, B );
	};
	IC	void	mulA_43		( const Self &A )	// mul after (no projection)
	{
    	Self B; B.set( *this ); 	mul_43	( A, B );
    };
	IC	void	mulB_43		( const Self &B )	// mul before (no projection)
	{
		Self A; A.set( *this ); 	mul_43	( A, B );
	};
	IC	void	invert		( const Self &a )
	{	
		// faster than self-invert
		T fDetInv = ( a._11 * ( a._22 * a._33 - a._23 * a._32 ) -
			a._12 * ( a._21 * a._33 - a._23 * a._31 ) +
			a._13 * ( a._21 * a._32 - a._22 * a._31 ) );

		VERIFY(fabsf(fDetInv)>flt_zero);
		fDetInv=1.0f/fDetInv;

		_11 =  fDetInv * ( a._22 * a._33 - a._23 * a._32 );
		_12 = -fDetInv * ( a._12 * a._33 - a._13 * a._32 );
		_13 =  fDetInv * ( a._12 * a._23 - a._13 * a._22 );
		_14 = 0.0f;

		_21 = -fDetInv * ( a._21 * a._33 - a._23 * a._31 );
		_22 =  fDetInv * ( a._11 * a._33 - a._13 * a._31 );
		_23 = -fDetInv * ( a._11 * a._23 - a._13 * a._21 );
		_24 = 0.0f;

		_31 =  fDetInv * ( a._21 * a._32 - a._22 * a._31 );
		_32 = -fDetInv * ( a._11 * a._32 - a._12 * a._31 );
		_33 =  fDetInv * ( a._11 * a._22 - a._12 * a._21 );
		_34 = 0.0f;

		_41 = -( a._41 * _11 + a._42 * _21 + a._43 * _31 );
		_42 = -( a._41 * _12 + a._42 * _22 + a._43 * _32 );
		_43 = -( a._41 * _13 + a._42 * _23 + a._43 * _33 );
		_44 = 1.0f;
	}

	IC	void	invert		()					// slower than invert other matrix
	{
		Self a;	a.set(*this);	invert(a);
	}
	IC	void	transpose	(const Self &matSource)	// faster version of transpose
	{
		_11=matSource._11;	_12=matSource._21;	_13=matSource._31;	_14=matSource._41;
		_21=matSource._12;	_22=matSource._22;	_23=matSource._32;	_24=matSource._42;
		_31=matSource._13;	_32=matSource._23;	_33=matSource._33;	_34=matSource._43;
		_41=matSource._14;	_42=matSource._24;	_43=matSource._34;	_44=matSource._44;
	}
	IC	void	transpose	()						// self transpose - slower
	{
		Self a;	a.set(*this);	transpose(a);
	}
	IC	void	translate	(const Tvector &Loc )		// setup translation matrix
	{	identity();	c.set	(Loc.x,Loc.y,Loc.z);	}
	IC	void	translate	(T _x, T _y, T _z ) // setup translation matrix
	{	identity(); c.set	(_x,_y,_z);				}
	IC	void	translate_over(const Tvector &Loc )	// modify only translation
	{	c.set	(Loc.x,Loc.y,Loc.z);				}
	IC	void	translate_over(T _x, T _y, T _z) // modify only translation
	{	c.set	(_x,_y,_z);							}
	IC	void	translate_add(const Tvector &Loc )	// combine translation
	{	c.add	(Loc.x,Loc.y,Loc.z);				}
	IC	void	scale		(T x, T y, T z )	// setup scale matrix
	{	identity(); m[0][0]=x; m[1][1]=y; m[2][2]=z; }
	IC	void	scale(const Tvector &v )			// setup scale matrix
	{	scale(v.x,v.y,v.z); }
	IC	void	scale_over(T x, T y, T z )// modify scaling
	{   m[0][0]=x;	m[1][1]=y;	m[2][2]=z;	}
	IC	void	scale_over(const Tvector &v )		// modify scaling
	{	scale_over(v.x,v.y,v.z); }
	IC	void	scale_add(const Tvector &v )		// combine scaling
	{	m[0][0]*=v.x; m[1][1]*=v.y;	m[2][2]*=v.z; }

	IC	void	rotateX		(T Angle )				// rotation about X axis
	{
		T cosa	= cosf(Angle);
		T sina	= sinf(Angle);
		i.set		(1,		0,		0	);	_14 = 0;
		j.set		(0,		cosa,	sina);	_24 = 0;
		k.set		(0,    -sina,   cosa);	_34 = 0;
		c.set		(0,		0,		0	);	_44 = 1;
	}
	IC	void	rotateY		(T Angle )				// rotation about Y axis
	{
		T cosa	= cosf(Angle);
		T sina	= sinf(Angle);
		i.set		(cosa,	0,	   -sina);	_14 = 0;
		j.set		(0,		1,		0	);	_24 = 0;
		k.set		(sina,  0,		cosa);	_34 = 0;
		c.set		(0,		0,		0	);	_44 = 1;
	}
	IC	void	rotateZ		(T Angle )				// rotation about Z axis
	{
		T cosa	= cosf(Angle);
		T sina	= sinf(Angle);
		i.set		(cosa,	sina,	0	);	_14 = 0;
		j.set		(-sina,	cosa,	0	);	_24 = 0;
		k.set		(0,		0,		1	);	_34 = 0;
		c.set		(0,		0,		0	);	_44 = 1;
	}

	IC	void	rotation	( const Tvector &vdir, const Tvector &vnorm )
	{
		Tvector vright;
		vright.crossproduct	(vnorm,vdir);
		vright.normalize	();
		m[0][0] = vright.x;	m[0][1] = vright.y;	m[0][2] = vright.z; m[0][3]=0;
		m[1][0] = vnorm.x;	m[1][1] = vnorm.y;	m[1][2] = vnorm.z;	m[1][3]=0;
		m[2][0] = vdir.x;	m[2][1] = vdir.y;	m[2][2] = vdir.z;	m[2][3]=0;
		m[3][0] = 0;		m[3][1] = 0;		m[3][2] = 0;		m[3][3]=1;
	}

	IC	void	mapXYZ		()	{i.set(1, 0, 0);_14=0;j.set(0, 1, 0);_24=0;k.set(0, 0, 1);_34=0;c.set(0, 0, 0);_44=1;	}
	IC	void	mapXZY		()	{i.set(1, 0, 0);_14=0;j.set(0, 0, 1);_24=0;k.set(0, 1, 0);_34=0;c.set(0, 0, 0);_44=1;	}
	IC	void	mapYXZ		()	{i.set(0, 1, 0);_14=0;j.set(1, 0, 0);_24=0;k.set(0, 0, 1);_34=0;c.set(0, 0, 0);_44=1;	}
	IC	void	mapYZX		()	{i.set(0, 1, 0);_14=0;j.set(0, 0, 1);_24=0;k.set(1, 0, 0);_34=0;c.set(0, 0, 0);_44=1;	}
	IC	void	mapZXY		()	{i.set(0, 0, 1);_14=0;j.set(1, 0, 0);_24=0;k.set(0, 1, 0);_34=0;c.set(0, 0, 0);_44=1;	}
	IC	void	mapZYX		()	{i.set(0, 0, 1);_14=0;j.set(0, 1, 0);_24=0;k.set(1, 0, 0);_34=0;c.set(0, 0, 0);_44=1;	}
	
	IC	void	rotation	( const Tvector &axis, T Angle )
	{
		T Cosine	= cosf(Angle);
		T Sine		= sinf(Angle);
		m [0][0] 	= axis.x * axis.x + ( 1 - axis.x * axis.x) * Cosine;
		m [0][1] 	= axis.x * axis.y * ( 1 - Cosine ) + axis.z * Sine;
		m [0][2] 	= axis.x * axis.z * ( 1 - Cosine ) - axis.y * Sine;
		m [0][3] 	= 0;
		m [1][0] 	= axis.x * axis.y * ( 1 - Cosine ) - axis.z * Sine;
		m [1][1] 	= axis.y * axis.y + ( 1 - axis.y * axis.y) * Cosine;
		m [1][2] 	= axis.y * axis.z * ( 1 - Cosine ) + axis.x * Sine;
		m [1][3] 	= 0;
		m [2][0] 	= axis.x * axis.z * ( 1 - Cosine ) + axis.y * Sine;
		m [2][1] 	= axis.y * axis.z * ( 1 - Cosine ) - axis.x * Sine;
		m [2][2] 	= axis.z * axis.z + ( 1 - axis.z * axis.z) * Cosine;
		m [2][3] 	= 0; m [3][0] = 0; m [3][1] = 0;
		m [3][2] 	= 0; m [3][3] = 1;
	}

	// mirror X
	IC	void	mirrorX ()
	{	identity();	m[0][0] = -1;	}
	IC	void	mirrorX_over ()
	{	m[0][0] = -1;	}
	IC	void	mirrorX_add ()
	{	m[0][0] *= -1;	}

	// mirror Y
	IC	void	mirrorY ()
	{	identity();	m [1][1] = -1;	}
	IC	void	mirrorY_over ()
	{	m [1][1] = -1;	}
	IC	void	mirrorY_add ()
	{	m [1][1] *= -1;	}

	// mirror Z
	IC	void	mirrorZ ()
	{	identity();		m [2][2] = -1;	}
	IC	void	mirrorZ_over ()
	{	m [2][2] = -1;	}
	IC	void	mirrorZ_add ()
	{	m [2][2] *= -1;	}

	IC	void	mul( const Self &A, T v )
	{
		m[0][0] = A.m [0][0] * v;	m[0][1] = A.m [0][1] * v;	m[0][2] = A.m [0][2] * v;	m[0][3] = A.m [0][3] * v;
		m[1][0] = A.m [1][0] * v;	m[1][1] = A.m [1][1] * v;	m[1][2] = A.m [1][2] * v;	m[1][3] = A.m [1][3] * v;
		m[2][0] = A.m [2][0] * v;	m[2][1] = A.m [2][1] * v;	m[2][2] = A.m [2][2] * v;	m[2][3] = A.m [2][3] * v;
		m[3][0] = A.m [3][0] * v;	m[3][1] = A.m [3][1] * v;	m[3][2] = A.m [3][2] * v;	m[3][3] = A.m [3][3] * v;
	}
	IC	void	mul( T v )
	{
		m[0][0] *= v;		m[0][1] *= v;		m[0][2] *= v;		m[0][3] *= v;
		m[1][0] *= v;		m[1][1] *= v;		m[1][2] *= v;		m[1][3] *= v;
		m[2][0] *= v;		m[2][1] *= v;		m[2][2] *= v;		m[2][3] *= v;
		m[3][0] *= v;		m[3][1] *= v;		m[3][2] *= v;		m[3][3] *= v;
	}
	IC	void	div( const Self &A, T v ) {
		VERIFY(fabsf(v)>0.000001f);
		mul(A,1.0f/v);
	}
	IC	void	div( T v ) {
		VERIFY(fabsf(v)>0.000001f);
		mul(1.0f/v);
	}
	// fov
	IC	void	build_projection		(T fFOV, T fAspect, T fNearPlane, T fFarPlane) {
		build_projection_HAT			(tanf(fFOV/2.f),fAspect,fNearPlane,fFarPlane);
	}
	// half_fov-angle-tangent
	IC	void	build_projection_HAT	(T HAT, T fAspect, T fNearPlane, T fFarPlane) {
		VERIFY( fabsf(fFarPlane-fNearPlane) > EPS_S );
		VERIFY( fabsf(HAT) > EPS_S );
		
		T cot	= 1/HAT;
		T w		= fAspect * cot;
		T h		= 1.0f    * cot;
		T Q		= fFarPlane / ( fFarPlane - fNearPlane );
		
		ZeroMemory	(this,sizeof(*this));
		_11			= w;
		_22			= h;
		_33			= Q;
		_34			= 1.0f;
		_43			= -Q*fNearPlane;
	}
	IC	void	build_camera(const Tvector &vFrom, const Tvector &vAt, const Tvector &vWorldUp) 
	{
		// Get the z basis vector, which points straight ahead. This is the
		// difference from the eyepoint to the lookat point.
		Tvector vView;
		vView.sub(vAt,vFrom);
		vView.normalize();

		// Get the dot product, and calculate the projection of the z basis
		// vector onto the up vector. The projection is the y basis vector.
		T fDotProduct = vWorldUp.dotproduct( vView );

		Tvector vUp;
		vUp.mul(vView, -fDotProduct);
		vUp.add(vWorldUp);
		vUp.normalize();

		// The x basis vector is found simply with the cross product of the y
		// and z basis vectors
		Tvector vRight;
		vRight.crossproduct( vUp, vView );

		// Start building the Device.mView. The first three rows contains the basis
		// vectors used to rotate the view to point at the lookat point
		_11 = vRight.x;  _12 = vUp.x;  _13 = vView.x;  _14 = 0.0f;
		_21 = vRight.y;  _22 = vUp.y;  _23 = vView.y;  _24 = 0.0f;
		_31 = vRight.z;  _32 = vUp.z;  _33 = vView.z;  _34 = 0.0f;

		// Do the translation values (rotations are still about the eyepoint)
		_41 = - vFrom.dotproduct(vRight);
		_42 = - vFrom.dotproduct( vUp  );
		_43 = - vFrom.dotproduct(vView );
		_44 = 1.0f;
	}
	IC	void	build_camera_dir(const Tvector &vFrom, const Tvector &vView, const Tvector &vWorldUp) 
	{
		// Get the dot product, and calculate the projection of the z basis
		// vector onto the up vector. The projection is the y basis vector.
		T fDotProduct = vWorldUp.dotproduct( vView );

		Tvector vUp;
		vUp.mul(vView, -fDotProduct);
		vUp.add(vWorldUp);
		vUp.normalize();

		// The x basis vector is found simply with the cross product of the y
		// and z basis vectors
		Tvector vRight;
		vRight.crossproduct( vUp, vView );

		// Start building the Device.mView. The first three rows contains the basis
		// vectors used to rotate the view to point at the lookat point
		_11 = vRight.x;  _12 = vUp.x;  _13 = vView.x;  _14 = 0.0f;
		_21 = vRight.y;  _22 = vUp.y;  _23 = vView.y;  _24 = 0.0f;
		_31 = vRight.z;  _32 = vUp.z;  _33 = vView.z;  _34 = 0.0f;

		// Do the translation values (rotations are still about the eyepoint)
		_41 = - vFrom.dotproduct(vRight);
		_42 = - vFrom.dotproduct( vUp  );
		_43 = - vFrom.dotproduct(vView );
		_44 = 1.0f;
	}

	IC	void	inertion(const Self &mat, T v)
	{
		T iv = 1.f-v;
		for (int i=0; i<4; i++)
		{
			m[i][0] = m[i][0]*v + mat.m[i][0]*iv;
			m[i][1] = m[i][1]*v + mat.m[i][1]*iv;
			m[i][2] = m[i][2]*v + mat.m[i][2]*iv;
			m[i][3] = m[i][3]*v + mat.m[i][3]*iv;
		}
	}
	IC	void	transform_tiny		(Tvector &dest, const Tvector &v)	const // preferred to use
	{
		dest.x = v.x*_11 + v.y*_21 + v.z*_31 + _41;
		dest.y = v.x*_12 + v.y*_22 + v.z*_32 + _42;
		dest.z = v.x*_13 + v.y*_23 + v.z*_33 + _43;
	}
	IC	void	transform_tiny32	(Fvector2 &dest, const Tvector &v)	const // preferred to use
	{
		dest.x = v.x*_11 + v.y*_21 + v.z*_31 + _41;
		dest.y = v.x*_12 + v.y*_22 + v.z*_32 + _42;
	}
	IC	void	transform_tiny23	(Tvector &dest, const Fvector2 &v)	const // preferred to use
	{
		dest.x = v.x*_11 + v.y*_21 + _41;
		dest.y = v.x*_12 + v.y*_22 + _42;
		dest.z = v.x*_13 + v.y*_23 + _43;
	}
	IC	void	transform_dir		(Tvector &dest, const Tvector &v)	const 	// preferred to use
	{
		dest.x = v.x*_11 + v.y*_21 + v.z*_31;
		dest.y = v.x*_12 + v.y*_22 + v.z*_32;
		dest.z = v.x*_13 + v.y*_23 + v.z*_33;
	}
	IC	void	transform			(Fvector4 &dest, const Tvector &v)	const 	// preferred to use
	{
		dest.w = v.x*_14 + v.y*_24 + v.z*_34 + _44;
		dest.x = (v.x*_11 + v.y*_21 + v.z*_31 + _41)/dest.w;
		dest.y = (v.x*_12 + v.y*_22 + v.z*_32 + _42)/dest.w;
		dest.z = (v.x*_13 + v.y*_23 + v.z*_33 + _43)/dest.w;
	}
	IC	void	transform			(Tvector &dest, const Tvector &v)	const 	// preferred to use
	{
		T w	= v.x*_14 + v.y*_24 + v.z*_34 + _44;
		dest.x	= (v.x*_11 + v.y*_21 + v.z*_31 + _41)/w;
		dest.y	= (v.x*_12 + v.y*_22 + v.z*_32 + _42)/w;
		dest.z	= (v.x*_13 + v.y*_23 + v.z*_33 + _43)/w;
	}

	IC	void	transform_tiny		(Tvector &v) const
	{
		Tvector			res;
		transform_tiny	(res,v);
		v.set			(res);
	}
	IC	void	transform			(Tvector &v) const
	{
		Tvector			res;
		transform		(res,v);
		v.set			(res);
	}
	IC	void	transform_dir		(Tvector &v) const
	{
		Tvector			res;
		transform_dir	(res,v);
		v.set			(res);
	}
	IC	void	setHPB	(T h, T p, T b)
	{
        T _ch, _cp, _cb, _sh, _sp, _sb, _cc, _cs, _sc, _ss;

        _sincos(h,_sh,_ch);
        _sincos(p,_sp,_cp);
        _sincos(b,_sb,_cb);
        _cc = _ch*_cb; _cs = _ch*_sb; _sc = _sh*_cb; _ss = _sh*_sb;

        i.set(_cc-_sp*_ss,	-_cp*_sb,	_sp*_cs+_sc	);	_14_=0;
        j.set(_sp*_sc+_cs,	_cp*_cb, 	_ss-_sp*_cc	);	_24_=0;
        k.set(-_cp*_sh,    	_sp,		_cp*_ch		);	_34_=0;
        c.set(0,			0,			0			);  _44_=1;
    }
	IC	void	setXYZ	(T x, T y, T z){setHPB(y,x,z);}
	IC	void	getHPB	(T& h, T& p, T& b){
        T cy = _sqrt(j.y*j.y + i.y*i.y);
        if (cy > 16.0f*FLT_EPSILON) {
            h = -atan2(k.x, k.z);
            p = -atan2(-k.y, cy);
            b = -atan2(i.y, j.y);
        } else {
            h = -atan2(-i.z, i.x);
            p = -atan2(-k.y, cy);
            b = 0;
        }
    }
	IC	void	getXYZ	(T& x, T& y, T& z){getHPB(y,x,z);}
	IC	void	getHPB	(Tvector& hpb){getHPB(hpb.x,hpb.y,hpb.z);}
	IC	void	getXYZ	(Tvector& xyz){getXYZ(xyz.x,xyz.y,xyz.z);}
    // remove
	IC	void	setHPB_old	(T h, T p, T b)
	{
		T _sp,_cp;		_sincos(_p,_sp,_cp);
		T _sh,_ch;		_sincos(_h,_sh,_ch);
		T _sb,_cb;		_sincos(_b,_sb,_cb);

		i.set(_cb*_ch+_sb*_sp*_sh, 	_sb*_cp,	_sh*_cb-_sb*_sp*_ch);	_14_=0;
		j.set(-_sb*_ch+_cb*_sp*_sh, _cb*_cp,	-_sb*_sh-_cb*_sp*_ch);	_24_=0;
		k.set(-_cp*_sh, 			_sp, 		_cp*_ch);				_34_=0;
		c.set(0,					0,			0);						_44_=1;
    }
	IC	void	getHPB_old	(T& h, T& p, T& b){
        if ( m[2][1] < 1.0f ){
            if ( m[2][1] > -1.0f ){
                b 	= -atan2(-m[0][1],m[1][1]);
                p 	= asin(m[2][1]);
                h 	= atan2(-m[2][0],m[2][2]);
            }else{
                b 	= atan2(m[0][2],m[0][0]);
                p 	= -PI_DIV_2;
                h	= 0.0f;
            }
        }else{
            b 	= -atan2(m[0][2],m[0][0]);
            p 	= PI_DIV_2;
            h 	= 0.0f;
        }
    }
};

typedef		_matrix<float>	Fmatrix;
typedef		_matrix<double>	Dmatrix;

extern ENGINE_API Fmatrix	Fidentity;
extern ENGINE_API Dmatrix	Didentity;

#endif

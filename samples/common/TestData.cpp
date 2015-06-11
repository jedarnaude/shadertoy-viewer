#include "ShadertoyTestHelper.h"

static ShadertoyTest mouse_test = {
    "Mouse test.",
    // https://www.shadertoy.com/view/Mss3zH
    // Image pass
    {
        R"(
                // Created by inigo quilez - iq/2013
                // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

                    float distanceToSegment( vec2 a, vec2 b, vec2 p )
                {
	                vec2 pa = p - a;
	                vec2 ba = b - a;
	                float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
	                return length( pa - ba*h );
                }

                    void mainImage( out vec4 fragColor, in vec2 fragCoord )
                {
	                vec2 p = fragCoord.xy / iResolution.xx;
                    vec4 m = iMouse / iResolution.xxxx;
	
	                vec3 col = vec3(0.0);

        		                if( m.z>0.0 )
	                {
		                float d = distanceToSegment( m.xy, m.zw, p );
                        col = mix( col, vec3(1.0,1.0,0.0), 1.0-smoothstep(.005,0.006, d) );
	                }

        		                col = mix( col, vec3(1.0,0.0,0.0), 1.0-smoothstep(.03,0.04, length(p-m.xy)) );
                    col = mix( col, vec3(0.0,0.0,1.0), 1.0-smoothstep(.03,0.04, length(p-abs(m.zw))) );

        		                fragColor = vec4( col, 1.0 );
                }
            )",
            {}
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest resolution_test = {
    "Resolution test.",
    // Image pass
    {
        R"(
            // Created by inigo quilez - iq/2014
            // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

            float hash1( float p ) { return fract(sin(p)*43758.5453); }
            vec2  hash2( float p ) { vec2 q = vec2( p, p+123.123 ); return fract(sin(q)*43758.5453); }
            vec3  hash3( float n ) { return fract(sin(vec3(n,n+1.0,n+2.0))*43758.5453123); }

            // draw a disk with motion blur
            vec3 diskWithMotionBlur( vec3 col, in vec2 uv, in vec3 sph, in vec2 cd, in vec3 sphcol, in float alpha )
            {
	            vec2 xc = uv - sph.xy;
	            float a = dot(cd,cd);
	            float b = dot(cd,xc);
	            float c = dot(xc,xc) - sph.z*sph.z;
	            float h = b*b - a*c;
	            if( h>0.0 )
	            {
		            h = sqrt( h );
		
		            float ta = max( 0.0, (-b - h)/a );
		            float tb = min( 1.0, (-b + h)/a );
		
		            if( ta < tb ) // we can comment this conditional, in fact
		                col = mix( col, sphcol, alpha*clamp(2.0*(tb-ta),0.0,1.0) );
	            }

    		            return col;
            }

            vec2 GetPos( in vec2 p, in vec2 v, in vec2 a, float t )
            {
	            return p + v*t + 0.5*a*t*t;
            }
            vec2 GetVel( in vec2 p, in vec2 v, in vec2 a, float t )
            {
	            return v + a*t;
            }

            // intersect a disk moving in a parabolic trajecgory with a line/plane. 
            // sphere is |x(t)|-R²=0, with x(t) = p + v·t + ½·a·t²
            // plane is <x,n> + k = 0
            float iPlane( in vec2 p, in vec2 v, in vec2 a, float rad, vec3 pla )
            {
	            float A = dot(a,pla.xy);
	            float B = dot(v,pla.xy);
	            float C = dot(p,pla.xy) + pla.z - rad;
	            float h = B*B - 2.0*A*C;
	            if( h>0.0 )
		            h = (-B-sqrt(h))/A;
	            return h;
            }

            const vec2 acc = vec2(0.01,-9.0);

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
	            vec2 p = -1.0 + 2.0*fragCoord.xy / iResolution.xy;
	            p.x *= iResolution.x/iResolution.y;
	            float w = iResolution.x/iResolution.y;

    	                vec3 pla0 = vec3( 0.0,1.0,1.0);
                vec3 pla1 = vec3(-1.0,0.0,  w);	
                vec3 pla2 = vec3( 1.0,0.0,  w);
		
	            vec3 col = vec3(0.0) + (0.15 + 0.05*p.y);
	
	            for( int i=0; i<8; i++ )
                {
                    // start position		
		            float id = float(i);

    			                float time = mod( iGlobalTime + id*0.5, 4.8 );
	                float sequ = floor( (iGlobalTime+id*0.5)/4.8 );
		            float life = time/4.8;

    				            float rad = 0.05 + 0.1*hash1(id*13.0 + sequ);
		            vec2 pos = vec2(-w,0.8) + vec2(2.0*w,0.2)*hash2( id + sequ );
		            vec2 vel = (-1.0 + 2.0*hash2( id+13.76 + sequ ))*vec2(8.0,1.0);

    		                    // integrate
		            float h = 0.0;
		            // 10 bounces. 
                    // after the loop, pos and vel contain the position and velocity of the ball
                    // after the last collision, and h contains the time since that collision.
	                for( int j=0; j<10; j++ )
	                {
			            float ih = 100000.0;
			            vec2 nor = vec2(0.0,1.0);

    						            // intersect planes
			            float s;
			            s = iPlane( pos, vel, acc, rad, pla0 ); if( s>0.0 && s<ih ) { ih=s; nor=pla0.xy; }
			            s = iPlane( pos, vel, acc, rad, pla1 ); if( s>0.0 && s<ih ) { ih=s; nor=pla1.xy; }
			            s = iPlane( pos, vel, acc, rad, pla2 ); if( s>0.0 && s<ih ) { ih=s; nor=pla2.xy; }
			
                        if( ih<1000.0 && (h+ih)<time )		
			            {
				            vec2 npos = GetPos( pos, vel, acc, ih );
				            vec2 nvel = GetVel( pos, vel, acc, ih );
				            pos = npos;
				            vel = nvel;
				            vel = 0.75*reflect( vel, nor );
				            pos += 0.01*vel;
                            h += ih;
			            }
                    }
		
                    // last parabolic segment
		            h = time - h;
		            vec2 npos = GetPos( pos, vel, acc, h );
		            vec2 nvel = GetVel( pos, vel, acc, h );
		            pos = npos;
		            vel = nvel;

    		                    // render
		            vec3 scol = 0.5 + 0.5*sin( hash1(id)*2.0 -1.0 + vec3(0.0,2.0,4.0) );
		            float alpha = smoothstep(0.0,0.1,life)-smoothstep(0.8,1.0,life);
		            col = diskWithMotionBlur( col, p, vec3(pos,rad), vel/24.0, scol, alpha );
                }

    	                col += (1.0/255.0)*hash3(p.x+13.0*p.y);

    			            fragColor = vec4(col,1.0);
            }
            )",
            {}
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest textures_test = {
    "Textures test.",
    // https://www.shadertoy.com/view/4dSSWc
    // Image pass
    {
        R"(
            float PI = 3.14159265;

            float width = 0.1;

            void mainImage( out vec4 fragColor, in vec2 fragCoord ) {			
                vec2 pos = fragCoord.xy/iResolution.xy - vec2(0.5);
    
                vec3 col = vec3(0.0); //Will be filled with twister later on
    
                float a = (sin(iGlobalTime*1.4+pos.y)*3.0)*pos.y+iGlobalTime*1.2; //Rotation value
                vec4 twister = vec4(sin(a), sin(a+0.5*PI), sin(a+PI), sin(a+1.5*PI))*width; //Contains "vertices"
    
                vec4 alpha=vec4( //If here in x should be filled or not. Multiply color with this
                    (1.0-clamp(((pos.x-twister.x)*(pos.x-twister.y))*100000.0, 0.0, 1.0)),
                    (1.0-clamp(((pos.x-twister.y)*(pos.x-twister.z))*100000.0, 0.0, 1.0)),
                    (1.0-clamp(((pos.x-twister.z)*(pos.x-twister.w))*100000.0, 0.0, 1.0)),
                    (1.0-clamp(((pos.x-twister.w)*(pos.x-twister.x))*100000.0, 0.0, 1.0))
                );
    
                alpha *= vec4( //Test if line is facing the way it will be showing
    	            1.0-clamp((twister.x-twister.y)*10000.0, 0.0, 1.0),
    	            1.0-clamp((twister.y-twister.z)*10000.0, 0.0, 1.0),
    	            1.0-clamp((twister.z-twister.w)*10000.0, 0.0, 1.0),
    	            1.0-clamp((twister.w-twister.x)*10000.0, 0.0, 1.0)
                );

    	                vec4 shade=vec4(
    	            twister.y-twister.x,
    	            twister.z-twister.y,
    	            twister.w-twister.z,
    	            twister.x-twister.w
                );
    
                shade /= width*1.8;

    	            col.rgb += texture2D(iChannel0, vec2(((pos.x-((twister.x+twister.y)/2.0))/(twister.x-twister.y))*width, pos.y)).rgb * alpha.x * shade.x;
                col.rgb += texture2D(iChannel1, vec2(((pos.x-((twister.y+twister.z)/2.0))/(twister.y-twister.z))*width, pos.y)).rgb * alpha.y * shade.y;
                col.rgb += texture2D(iChannel2, vec2(((pos.x-((twister.z+twister.w)/2.0))/(twister.z-twister.w))*width, pos.y)).rgb * alpha.z * shade.z;
                col.rgb += texture2D(iChannel3, vec2(((pos.x-((twister.w+twister.x)/2.0))/(twister.w-twister.x))*width, pos.y)).rgb * alpha.w * shade.w;
    
                fragColor = vec4(col, 1.0);
            }
            )",
            {
                { SHADERTOY_RESOURCE_TEXTURE, { "tex00.jpg" } },
                { SHADERTOY_RESOURCE_TEXTURE, { "tex06.jpg" } },
                { SHADERTOY_RESOURCE_TEXTURE, { "tex02.jpg" } },
                { SHADERTOY_RESOURCE_TEXTURE, { "tex08.jpg" } },
            }
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest cubemap_test = {
    "Cubemap test.",
    // Image pass
    {
        R"(
            // Reprojection II - @P_Malin

            // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

            #define FORCE_SHADOW
            #define ENABLE_REFLECTION

            struct C_Ray
            {
                vec3 vOrigin;
                vec3 vDir;
            };
	
            #define kMaxDist 1000.0
            #define kEpsilon 0.0001
	
            void TraceSlab(const in C_Ray ray, const in vec3 vMin, const in vec3 vMax, const in vec3 vNormal, inout float fNear, inout float fFar)
            {
	            vec3 vMinOffset = vMin - ray.vOrigin;
	            vec3 vMaxOffset = vMax - ray.vOrigin;

		            // Project offset and dir
	            float fMinOffset = dot(vMinOffset, vNormal);	
	            float fMaxOffset = dot(vMaxOffset, vNormal);	
	
	            float fDir = dot(ray.vDir, vNormal);
		
	            if(abs(fDir) < kEpsilon)
	            {
		            // ray parallel to slab
		
		            //if origin is not between slabs return false;
		            if((fMinOffset > 0.0) || (fMaxOffset < 0.0))
		            {
			            fNear = kMaxDist;
			            fFar = -kMaxDist;
		            }
		
		            // else this slab does not influence the result
	            }
	            else
	            {
		            // ray is not parallel to slab, calculate intersections
		
		            float t0 = (fMinOffset) / fDir;
		            float t1 = (fMaxOffset) / fDir;
		
		            float fIntersectNear = min(t0, t1);
		            float fIntersectFar = max(t0, t1);
		
		            fNear = max(fNear, fIntersectNear); // track largest near
		            fFar = min(fFar, fIntersectFar); // track smallest far
	            }
            }
	
            float TraceBox( const in C_Ray ray, const in vec3 vCorner1, const in vec3 vCorner2 )
            {
	            vec3 vMin = min(vCorner1, vCorner2);
	            vec3 vMax = max(vCorner1, vCorner2);
	
	            float fNear = -kMaxDist;
	            float fFar = kMaxDist;
	
	            TraceSlab(ray, vMin, vMax, vec3(1.0, 0.0, 0.0), fNear, fFar);
	            TraceSlab(ray, vMin, vMax, vec3(0.0, 1.0, 0.0), fNear, fFar);
	            TraceSlab(ray, vMin, vMax, vec3(0.0, 0.0, 1.0), fNear, fFar);
	
	            if(fNear > fFar)
	            {
		            return kMaxDist;
	            }
	
	            if(fFar < 0.0)
	            {
		            return kMaxDist;
	            }
	
	            return fNear;
            }

            vec3 Project( vec3 a, vec3 b )
            {
	            return a - b * dot(a, b);
            }

            float TraceCylinder( const in C_Ray ray, vec3 vPos, vec3 vDir, float fRadius, float fLength )
            {	
	            vec3 vOffset = vPos - ray.vOrigin;
	
	            vec3 vProjOffset = Project(vOffset, vDir);
	            vec3 vProjDir = Project(ray.vDir, vDir);
	            float fProjScale = length(vProjDir);
	            vProjDir /= fProjScale;
	
	            // intersect circle in projected space
	
	            float fTClosest = dot(vProjOffset, vProjDir);
	
	            vec3 vClosest = vProjDir * fTClosest;
	            float fDistClosest = length(vClosest - vProjOffset);
	            if(fDistClosest > fRadius)
	            {
		            return kMaxDist;
	            }
	            float fHalfChordLength = sqrt(fRadius * fRadius - fDistClosest * fDistClosest);
	            float fTIntersectMin = (fTClosest - fHalfChordLength) / fProjScale;
	            float fTIntersectMax = (fTClosest + fHalfChordLength) / fProjScale;
	
	            // cap cylinder ends
	            TraceSlab(ray, vPos, vPos + vDir * fLength, vDir, fTIntersectMin, fTIntersectMax);

		            if(fTIntersectMin > fTIntersectMax)
	            {
		            return kMaxDist;
	            }
	
	            if(fTIntersectMin < 0.0)
	            {
		            return kMaxDist;
	            }
	
	            return fTIntersectMin;		
            }
			   

			               float TraceFloor( const in C_Ray ray, const in float fHeight )
            {
	            if(ray.vOrigin.y < fHeight)
	            {
		            return 0.0;
	            }
	
	            if(ray.vDir.y > 0.0)
	            {
		            return kMaxDist;
	            }
	
	            float t = (fHeight - ray.vOrigin.y) / ray.vDir.y;
	
	            return max(t, 0.0);
            }

            float TracePillar( const in C_Ray ray, in vec3 vPos )
            {
	            vPos.y = -1.0;
	            float fRadius = 0.3;
	            float fDistance = TraceCylinder( ray, vPos, vec3(0.0, 1.0, 0.0), fRadius, 10.0);
	            float fBaseSize = 0.4;
	
	            vec3 vBaseMin = vec3(-fBaseSize, 0.0, -fBaseSize);
	            vec3 vBaseMax = vec3(fBaseSize, 0.8, fBaseSize);
	            fDistance = min( fDistance, TraceBox( ray, vPos + vBaseMin, vPos + vBaseMax) );
	
	            float fTopSize = 0.4;
	            vec3 vTopMin = vec3(-fTopSize, 5.6, -fTopSize);
	            vec3 vTopMax = vec3(fTopSize, 7.0, fTopSize);
	            fDistance = min( fDistance, TraceBox( ray, vPos + vTopMin, vPos + vTopMax) );
	
	            return fDistance;	
            }

            float TraceColumn( const in C_Ray ray, vec3 vPos )
            {
	            vec3 vColumnMin = vec3(-0.84, -10.0, -0.4);
	            vec3 vColumnMax = - vColumnMin;
	            return TraceBox( ray, vPos + vColumnMin, vPos + vColumnMax );	
            }

            float fBuildingMin = -90.0;
            float fBuildingMax = 50.0;

            float TraceBuildingSide( const in C_Ray ray )
            {
	            float fDistance = kMaxDist;
	
	            float fStepHeight = 0.14;
	            float fStepDepth = 0.2;
	            float fStepStart = 7.5;
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMin, -1.5 + fStepHeight * 0.0, fStepStart + fStepDepth * 0.0), vec3(fBuildingMax, -1.5 + fStepHeight * 1.0, fStepStart + 20.0) ));
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMin, -1.5 + fStepHeight * 1.0, fStepStart + fStepDepth * 1.0), vec3(fBuildingMax, -1.5 + fStepHeight * 2.0, fStepStart + 20.0) ));
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMin, -1.5 + fStepHeight * 2.0, fStepStart + fStepDepth * 2.0), vec3(fBuildingMax, -1.5 + fStepHeight * 3.0, fStepStart + 20.0) ));
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMin, -1.5 + fStepHeight * 3.0, fStepStart + fStepDepth * 3.0), vec3(fBuildingMax, -1.5 + fStepHeight * 4.0, fStepStart + 20.0) ));

		            float x = -2.0;
	            for(int i=0; i<5; i++)
	            {
		            vec3 vBase = vec3(x * 11.6, 0.0, 0.0);
		            x += 1.0;
		
		            fDistance = min(fDistance, TraceColumn(ray, vBase + vec3(0.0, 0.0, 8.5)));
		
		
		            fDistance = min(fDistance, TracePillar(ray, vBase + vec3(-4.1, 0.0, 8.5)));	
		            fDistance = min(fDistance, TracePillar(ray, vBase + vec3(4.0, 0.0, 8.5)));
	            }
	

		            float fBackWallDist = 9.5;
	            float fBuildingHeight = 100.0;
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMin, -3.0, fBackWallDist), vec3(fBuildingMax, fBuildingHeight, fBackWallDist + 10.0) ));

		            float fBuildingTopDist = 8.1;
	            float fCeilingHeight = 4.7;
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMin, fCeilingHeight, fBuildingTopDist), vec3(fBuildingMax, fBuildingHeight, fBuildingTopDist + 10.0) ));

		            float fRoofDistance = 6.0;
	            float fRoofHeight = 21.0;
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMin, fRoofHeight, fRoofDistance), vec3(fBuildingMax, fRoofHeight + 0.2, fRoofDistance + 10.0) ));	
	
	            return fDistance;
            }
	
            float TraceScene( const in C_Ray ray )
            {        
                float fDistance = kMaxDist;
        
	            float fFloorHeight = -1.5;
	            fDistance = min(fDistance, TraceFloor( ray, fFloorHeight ));
	
	            // end of row
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMax, fFloorHeight, -100.0), vec3(fBuildingMax+1.0, 100.0, 100.0) ));
	            fDistance = min(fDistance, TraceBox( ray, vec3(fBuildingMin, fFloorHeight, -100.0), vec3(fBuildingMin-1.0, 100.0, 100.0) ));
		
	            fDistance = min(fDistance, TraceBuildingSide( ray ));
	
	            C_Ray ray2;					
	            ray2.vOrigin = ray.vOrigin * vec3(1.0, 1.0, -1.0);
	            ray2.vDir = ray.vDir * vec3(1.0, 1.0, -1.0);
	            ray2.vOrigin.z -= 0.3;
	            fDistance = min(fDistance, TraceBuildingSide( ray2 ));
					
	            return fDistance;
            }
               

			               void GetCameraRay( const in vec3 vPos, const in vec3 vForwards, const in vec3 vWorldUp, const in vec2 px, out C_Ray ray)
            {
                vec2 vUV = ( px / iResolution.xy );
                vec2 vViewCoord = vUV * 2.0 - 1.0;	

		            vViewCoord.x *= iResolution.x / iResolution.y;
	            vViewCoord.y *= -1.0;

	                ray.vOrigin = vPos;

		                vec3 vRight = normalize(cross(vWorldUp, vForwards));
                vec3 vUp = cross(vRight, vForwards);
    
	            vViewCoord *= 0.5;
	
                ray.vDir = normalize( vRight * vViewCoord.x + vUp * vViewCoord.y + vForwards);    
            }

            void GetCameraRayLookat( const in vec3 vPos, const in vec3 vCameraTarget, const in vec2 px, out C_Ray ray)
            {
	            vec3 vForwards = normalize(vCameraTarget - vPos);
	            vec3 vUp = vec3(0.0, 1.0, 0.0);

		            GetCameraRay(vPos, vForwards, vUp, px, ray);
            }

            void GetCameraPosAndTarget( float fCameraIndex, out vec3 vCameraPos, out vec3 vCameraTarget )
            {
	            float fCameraCount = 6.0;
	            float fCameraIndexModCount = mod(fCameraIndex, fCameraCount);

		            if(fCameraIndexModCount < 0.5)
	            {
		            vCameraPos = vec3(0.0, 0.0, 0.0);
		            vCameraTarget = vec3(0.0, 0.0, 8.0);
	            }
	            else if(fCameraIndexModCount < 1.5)
	            {
		            vCameraPos = vec3(-3.0, 0.0, -5.0);
		            vCameraTarget = vec3(3.0, -5.0, 5.0);
	            }
	            else if(fCameraIndexModCount < 2.5)
	            {
		            vCameraPos = vec3(8.0, 0.0, 0.0);
		            vCameraTarget = vec3(-10.0, 0.0, 0.0);
	            }
	            else if(fCameraIndexModCount < 3.5)
	            {
		            vCameraPos = vec3(8.0, 3.0, -3.0);
		            vCameraTarget = vec3(-4.0, -2.0, 0.0);
	            }
	            else if(fCameraIndexModCount < 4.5)
	            {
		            vCameraPos = vec3(8.0, 5.0, 5.0);
		            vCameraTarget = vec3(-4.0, 2.0, -5.0);
	            }
	            else
	            {
		            vCameraPos = vec3(-10.0, 3.0, 0.0);
		            vCameraTarget = vec3(4.0, 4.5, -5.0);
	            }
            }

            vec3 BSpline( const in vec3 a, const in vec3 b, const in vec3 c, const in vec3 d, const in float t)
            {
	            const mat4 mSplineBasis = mat4( -1.0,  3.0, -3.0, 1.0,
							                     3.0, -6.0,  0.0, 4.0,
							                    -3.0,  3.0,  3.0, 1.0,
							                     1.0,  0.0,  0.0, 0.0) / 6.0;	
	
	            float t2 = t * t;
	            vec4 T = vec4(t2 * t, t2, t, 1.0);
	
	            vec4 vCoeffsX = vec4(a.x, b.x, c.x, d.x);
	            vec4 vCoeffsY = vec4(a.y, b.y, c.y, d.y);
	            vec4 vCoeffsZ = vec4(a.z, b.z, c.z, d.z);
	
	            vec4 vWeights = T * mSplineBasis;
	
	            vec3 vResult;
	
	            vResult.x = dot(vWeights, vCoeffsX);
	            vResult.y = dot(vWeights, vCoeffsY);
	            vResult.z = dot(vWeights, vCoeffsZ);
	
	            return vResult;
            }

            void GetCamera(out vec3 vCameraPos, out vec3 vCameraTarget)
            {
	            float fCameraGlobalTime = iGlobalTime * 0.5;
	            float fCameraTime = fract(fCameraGlobalTime);
	            float fCameraIndex = floor(fCameraGlobalTime);
	
	            vec3 vCameraPosA;
	            vec3 vCameraTargetA;
	            GetCameraPosAndTarget(fCameraIndex, vCameraPosA, vCameraTargetA);
	
	            vec3 vCameraPosB;
	            vec3 vCameraTargetB;
	            GetCameraPosAndTarget(fCameraIndex + 1.0, vCameraPosB, vCameraTargetB);
	
	            vec3 vCameraPosC;
	            vec3 vCameraTargetC;
	            GetCameraPosAndTarget(fCameraIndex + 2.0, vCameraPosC, vCameraTargetC);
	
	            vec3 vCameraPosD;
	            vec3 vCameraTargetD;
	            GetCameraPosAndTarget(fCameraIndex + 3.0, vCameraPosD, vCameraTargetD);
	
	            vCameraPos = BSpline(vCameraPosA, vCameraPosB, vCameraPosC, vCameraPosD, fCameraTime);
	            vCameraTarget = BSpline(vCameraTargetA, vCameraTargetB, vCameraTargetC, vCameraTargetD, fCameraTime);
            }

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
                C_Ray ray;

		            vec3 vResult = vec3(0.0);
	
	            vec2 vMouse = iMouse.xy / iResolution.xy;	
	
	            vec3 vCameraPos;
	            vec3 vCameraTarget;
	
	            GetCamera(vCameraPos, vCameraTarget);		

		            GetCameraRayLookat( vCameraPos, vCameraTarget, fragCoord, ray);
	
                float fHitDist = TraceScene(ray);
	            vec3 vHitPos = ray.vOrigin + ray.vDir * fHitDist;
	
	            vResult = textureCube(iChannel0, vHitPos.xyz).rgb;	
	            vResult = vResult * vResult;
	
	            #ifdef FORCE_SHADOW
	            if( abs(vHitPos.z) > 9.48)
	            {
		            if( abs(vHitPos.x) < 20.0)
		            {
			            float fIntensity = length(vResult);
			
			            fIntensity = min(fIntensity, 0.05);
			
			            vResult = normalize(vResult) * fIntensity;
		            }
	            }
	            #endif	
	
	            #ifdef ENABLE_REFLECTION
	            if(vHitPos.y < -1.4)
	            {
		            float fDelta = -0.1;
		            float vSampleDx = textureCube(iChannel0, vHitPos.xyz + vec3(fDelta, 0.0, 0.0)).r;	
		            vSampleDx = vSampleDx * vSampleDx;

				            float vSampleDy = textureCube(iChannel0, vHitPos.xyz + vec3(0.0, 0.0, fDelta)).r;	
		            vSampleDy = vSampleDy * vSampleDy;
		
		            vec3 vNormal = vec3(vResult.r - vSampleDx, 2.0, vResult.r - vSampleDy);
		            vNormal = normalize(vNormal);
		
		            vec3 vReflect = reflect(ray.vDir, vNormal);
		
		            float fDot = clamp(dot(-ray.vDir, vNormal), 0.0, 1.0);
		
		            float r0 = 0.1;
		            float fSchlick =r0 + (1.0 - r0) * (pow(1.0 - fDot, 5.0));
		
		            vec3 vResult2 = textureCube(iChannel1, vReflect).rgb;	
		            vResult2 = vResult2 * vResult2;
		            float shade = smoothstep(0.3, 0.0, vResult.r);
		            vResult += shade * vResult2 * fSchlick * 5.0;
	            }
	            #endif
	
	            if(iMouse.z > 0.0)
	            {
		            vec3 vGrid =  step(vec3(0.9), fract(vHitPos + 0.01));
		            float fGrid = min(dot(vGrid, vec3(1.0)), 1.0);
		            vResult = mix(vResult, vec3(0.0, 0.0, 1.0), fGrid);
	            }
	
	            fragColor = vec4(sqrt(vResult),1.0);
            }
            )",
            {
                { SHADERTOY_RESOURCE_CUBE_MAP, { "cube00_0.jpg", "cube00_1.jpg", "cube00_2.jpg", "cube00_3.jpg", "cube00_4.jpg", "cube00_5.jpg" } },
                { SHADERTOY_RESOURCE_CUBE_MAP, { "cube01_0.png", "cube01_1.png", "cube01_2.png", "cube01_3.png", "cube01_4.png", "cube01_5.png" } },
                {},
                {}
            }
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest date_test = {
    "Date test.",
    //https://www.shadertoy.com/view/4sBSWW
    // Image pass
    {
        R"(
            // Smaller Number Printing - @P_Malin
            // Creative Commons CC0 1.0 Universal (CC-0)

            // Feel free to modify, distribute or use in commercial code, just don't hold me liable for anything bad that happens!
            // If you use this code and want to give credit, that would be nice but you don't have to.

            // I first made this number printing code in https://www.shadertoy.com/view/4sf3RN
            // It started as a silly way of representing digits with rectangles.
            // As people started actually using this in a number of places I thought I would try to condense the 
            // useful function a little so that it can be dropped into other shaders more easily,
            // just snip between the perforations below.
            // Also, the licence on the previous shader was a bit restrictive for utility code.
            //
            // Note that the values printed are not always accurate!

            // ---- 8< ---- GLSL Number Printing - @P_Malin ---- 8< ----
            // Creative Commons CC0 1.0 Universal (CC-0) 
            // https://www.shadertoy.com/view/4sBSWW

            float DigitBin(const in int x)
            {
                return x==0?480599.0:x==1?139810.0:x==2?476951.0:x==3?476999.0:x==4?350020.0:x==5?464711.0:x==6?464727.0:x==7?476228.0:x==8?481111.0:x==9?481095.0:0.0;
            }

            float PrintValue(const in vec2 fragCoord, const in vec2 vPixelCoords, const in vec2 vFontSize, const in float fValue, const in float fMaxDigits, const in float fDecimalPlaces)
            {
                vec2 vStringCharCoords = (fragCoord.xy - vPixelCoords) / vFontSize;
                if ((vStringCharCoords.y < 0.0) || (vStringCharCoords.y >= 1.0)) return 0.0;
	            float fLog10Value = log2(abs(fValue)) / log2(10.0);
	            float fBiggestIndex = max(floor(fLog10Value), 0.0);
	            float fDigitIndex = fMaxDigits - floor(vStringCharCoords.x);
	            float fCharBin = 0.0;
	            if(fDigitIndex > (-fDecimalPlaces - 1.01)) {
		            if(fDigitIndex > fBiggestIndex) {
			            if((fValue < 0.0) && (fDigitIndex < (fBiggestIndex+1.5))) fCharBin = 1792.0;
		            } else {		
			            if(fDigitIndex == -1.0) {
				            if(fDecimalPlaces > 0.0) fCharBin = 2.0;
			            } else {
				            if(fDigitIndex < 0.0) fDigitIndex += 1.0;
				            float fDigitValue = (abs(fValue / (pow(10.0, fDigitIndex))));
                            float kFix = 0.0001;
                            fCharBin = DigitBin(int(floor(mod(kFix+fDigitValue, 10.0))));
			            }		
		            }
	            }
                return floor(mod((fCharBin / pow(2.0, floor(fract(vStringCharCoords.x) * 4.0) + (floor(vStringCharCoords.y * 5.0) * 4.0))), 2.0));
            }

            // ---- 8< -------- 8< -------- 8< -------- 8< ----


            float GetCurve(float x)
            {
	            return sin( x * 3.14159 * 4.0 );
            }

            float GetCurveDeriv(float x) 
            { 
	            return 3.14159 * 4.0 * cos( x * 3.14159 * 4.0 ); 
            }

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
	            vec3 vColour = vec3(0.0);

    	            // Multiples of 4x5 work best
	            vec2 vFontSize = vec2(8.0, 15.0);

    	            // Draw Horizontal Line
	            if(abs(fragCoord.y - iResolution.y * 0.5) < 1.0)
	            {
		            vColour = vec3(0.25);
	            }
	
	            // Draw Sin Wave
	            // See the comment from iq or this page
	            // http://www.iquilezles.org/www/articles/distance/distance.htm
	            float fCurveX = fragCoord.x / iResolution.x;
	            float fSinY = (GetCurve(fCurveX) * 0.25 + 0.5) * iResolution.y;
	            float fSinYdX = (GetCurveDeriv(fCurveX) * 0.25) * iResolution.y / iResolution.x;
	            float fDistanceToCurve = abs(fSinY - fragCoord.y) / sqrt(1.0+fSinYdX*fSinYdX);
	            float fSetPixel = fDistanceToCurve - 1.0; // Add more thickness
	            vColour = mix(vec3(1.0, 0.0, 0.0), vColour, clamp(fSetPixel, 0.0, 1.0));	

    	            // Draw Sin Value	
	            float fValue4 = GetCurve(iMouse.x / iResolution.x);
	            float fPixelYCoord = (fValue4 * 0.25 + 0.5) * iResolution.y;
	
	            // Plot Point on Sin Wave
	            float fDistToPointA = length( vec2(iMouse.x, fPixelYCoord) - fragCoord.xy) - 4.0;
	            vColour = mix(vColour, vec3(0.0, 0.0, 1.0), (1.0 - clamp(fDistToPointA, 0.0, 1.0)));
	
	            // Plot Mouse Pos
	            float fDistToPointB = length( vec2(iMouse.x, iMouse.y) - fragCoord.xy) - 4.0;
	            vColour = mix(vColour, vec3(0.0, 1.0, 0.0), (1.0 - clamp(fDistToPointB, 0.0, 1.0)));
	
	            // Print Sin Value
	            vec2 vPixelCoord4 = vec2(iMouse.x, fPixelYCoord) + vec2(4.0, 4.0);
	            float fDigits = 1.0;
	            float fDecimalPlaces = 2.0;
	            float fIsDigit4 = PrintValue(fragCoord, vPixelCoord4, vFontSize, fValue4, fDigits, fDecimalPlaces);
	            vColour = mix( vColour, vec3(0.0, 0.0, 1.0), fIsDigit4);
	
	            // Print Shader Time
	            vec2 vPixelCoord1 = vec2(96.0, 5.0);
	            float fValue1 = iGlobalTime;
	            fDigits = 6.0;
	            float fIsDigit1 = PrintValue(fragCoord, vPixelCoord1, vFontSize, fValue1, fDigits, fDecimalPlaces);
	            vColour = mix( vColour, vec3(0.0, 1.0, 1.0), fIsDigit1);

    	            // Print Date
	            vColour = mix( vColour, vec3(1.0, 1.0, 0.0), PrintValue(fragCoord, vec2(0.0, 5.0), vFontSize, iDate.x, 4.0, 0.0));
	            vColour = mix( vColour, vec3(1.0, 1.0, 0.0), PrintValue(fragCoord, vec2(0.0 + 48.0, 5.0), vFontSize, iDate.y + 1.0, 2.0, 0.0));
	            vColour = mix( vColour, vec3(1.0, 1.0, 0.0), PrintValue(fragCoord, vec2(0.0 + 72.0, 5.0), vFontSize, iDate.z, 2.0, 0.0));

    	            // Draw Time
	            vColour = mix( vColour, vec3(1.0, 0.0, 1.0), PrintValue(fragCoord, vec2(184.0, 5.0), vFontSize, mod(iDate.w / (60.0 * 60.0), 12.0), 2.0, 0.0));
	            vColour = mix( vColour, vec3(1.0, 0.0, 1.0), PrintValue(fragCoord, vec2(184.0 + 24.0, 5.0), vFontSize, mod(iDate.w / 60.0, 60.0), 2.0, 0.0));
	            vColour = mix( vColour, vec3(1.0, 0.0, 1.0), PrintValue(fragCoord, vec2(184.0 + 48.0, 5.0), vFontSize, mod(iDate.w, 60.0), 2.0, 0.0));
	
	            if(iMouse.x > 0.0)
	            {
		            // Print Mouse X
		            vec2 vPixelCoord2 = iMouse.xy + vec2(-52.0, 6.0);
		            float fValue2 = iMouse.x / iResolution.x;
		            fDigits = 1.0;
		            fDecimalPlaces = 3.0;
		            float fIsDigit2 = PrintValue(fragCoord, vPixelCoord2, vFontSize, fValue2, fDigits, fDecimalPlaces);
		            vColour = mix( vColour, vec3(0.0, 1.0, 0.0), fIsDigit2);
		
		            // Print Mouse Y
		            vec2 vPixelCoord3 = iMouse.xy + vec2(0.0, 6.0);
		            float fValue3 = iMouse.y / iResolution.y;
		            fDigits = 1.0;
		            float fIsDigit3 = PrintValue(fragCoord, vPixelCoord3, vFontSize, fValue3, fDigits, fDecimalPlaces);
		            vColour = mix( vColour, vec3(0.0, 1.0, 0.0), fIsDigit3);
	            }
	
	            fragColor = vec4(vColour,1.0);
            }
            )",
            {}
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest channel_resolution_test = {
    "Channel resolution test.",
    // Image pass
    {
        R"(
            // Created by KakiZeOne/2014
            // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

            // Pixel 2 Channel point formula
            #define PXL2Channel(V,t) vec2(V.x,iChannelResolution[t].y-V.y)/iChannelResolution[t].x

            // Set to use a none tile suitable picture (as London street 2D picture)
            #define PIXEL_LOOP

            // Set ZOOM_MIN == ZOOM_MAX to stop zooming
            #define ZOOM_MIN 0.5
            #define ZOOM_MAX 2.

            // Return iChannel color from pixel coordinates (use with tile suitable picture)
            vec4 iChannel0Pixel(vec2 v, float zoom) {
	            return texture2D(iChannel0, PXL2Channel((v/zoom),0));
            }
            // Return iChannel color from pixel coordinates (use with non tile suitable picture)
            vec4 iChannel0PixelLoop(vec2 v, float zoom) {
	            vec2 v2 = mod(v/zoom,2.*iChannelResolution[0].xy);
	            v2 = min(v2, vec2(2.*iChannelResolution[0].x-v2.x, 2.*iChannelResolution[0].y-v2.y));
	            return texture2D(iChannel0, PXL2Channel(v2,0));
            }

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
	            vec2 p = fragCoord.xy;

    	            #ifndef PIXEL_LOOP
	            fragColor = iChannel0Pixel(p, ZOOM_MIN
								               +(ZOOM_MAX-ZOOM_MIN)/2.*(1.-sin(iGlobalTime)));
            #else
	            fragColor = iChannel0PixelLoop(p, ZOOM_MIN
								 	               +(ZOOM_MAX-ZOOM_MIN)/2.*(1.-sin(iGlobalTime)));
            #endif
            }
            )",
            {
                { SHADERTOY_RESOURCE_TEXTURE, { "tex04.jpg" } },
                {},
                {},
                {}
            }
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest keyboard_test = {
    "Keyboard test.",
    //https://www.shadertoy.com/view/XsBXDw
    // Image pass
    {
        R"(
            // key is javascript keycode: http://www.webonweboff.com/tips/js/event_key_codes.aspx
            bool ReadKey( int key, bool toggle )
            {
	            float keyVal = texture2D( iChannel0, vec2( (float(key)+.5)/256.0, toggle?.75:.25 ) ).x;
	            return (keyVal>.5)?true:false;
            }

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
	            vec2 uv = fragCoord.xy / iResolution.xy;
                int keyNum = 65 + int(uv.x * (90.0 - 65.0));
                bool toggle = uv.y > 0.1;
    
                vec4 keyColor = ReadKey(keyNum, toggle) ? vec4(1.0 - uv.y, 0.0, 0.0, 1.0) : vec4(0.0, 1.0 - uv.y, 0.0, 1.0);

                        bool hozLine = abs(uv.x * 25.0 - float(int(uv.x * 25.0))) < 0.1;
                bool vertLine = abs(uv.y - 0.1) < 0.005;
    
	            fragColor = (hozLine || vertLine) ? vec4(0.0, 0.0, 0.0, 1.0) : keyColor;
            }
            )",
            {
                { SHADERTOY_RESOURCE_KEYBOARD, {} },
                {},
                {},
                {}
            }
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest derivatives_test = {
    "Derivatives test.",
    // Image pass
    {
        R"(
            // Created by inigo quilez - iq/2013
            // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
	            vec2 uv = fragCoord.xy / iResolution.xy;
	
	            vec3  col = texture2D( iChannel0, vec2(uv.x,1.0-uv.y) ).xyz;
	            float lum = dot(col,vec3(0.333));
	            vec3 ocol = col;
	
	            if( uv.x>0.5 )
	            {
		            // right side: changes in luminance
                    float f = fwidth( lum );
                    col *= 1.5*vec3( clamp(1.0-8.0*f,0.0,1.0) );
	            }
                else
	            {
		            // bottom left: emboss
                    vec3  nor = normalize( vec3( dFdx(lum), 64.0/iResolution.x, dFdy(lum) ) );
		            if( uv.y<0.5 )
		            {
			            float lig = 0.5 + dot(nor,vec3(0.7,0.2,-0.7));
                        col = vec3(lig);
		            }
		            // top left: bump
                    else
		            {
                        float lig = clamp( 0.5 + 1.5*dot(nor,vec3(0.7,0.2,-0.7)), 0.0, 1.0 );
                        col *= vec3(lig);
		            }
	            }

                    col *= smoothstep( 0.003, 0.004, abs(uv.x-0.5) );
	            col *= 1.0 - (1.0-smoothstep( 0.007, 0.008, abs(uv.y-0.5) ))*(1.0-smoothstep(0.49,0.5,uv.x));
                col = mix( col, ocol, pow( 0.5 + 0.5*sin(iGlobalTime), 4.0 ) );
	
	            fragColor = vec4( col, 1.0 );
            }            
            )",
            {
                { SHADERTOY_RESOURCE_TEXTURE,{ "tex04.jpg" } },
                {},
                {},
                {}
            }
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest sound_test = {
    "Sound pass test.",
    // Image pass
    {
        R"(
            // Created by inigo quilez - iq/2014
            // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
                vec2 p = (-iResolution.xy+2.0*fragCoord.xy)/iResolution.y;
	
                vec3 col = vec3( 0.0 );
	
                float noteA = fract( iGlobalTime/4.00 );
                float noteB = fract( iGlobalTime/0.50 );
                float noteC = fract( iGlobalTime/0.50 );
	
                col = mix( col, vec3(1.0,1.0,0.5), (1.0-noteA)*(1.0-smoothstep( 0.0, 0.5, length(p-vec2( 0.8,0.0)))));
                col = mix( col, vec3(0.5,1.0,0.5),      noteB *(1.0-smoothstep( 0.0, 0.2, length(p-vec2( 0.0,0.0)))));
                col = mix( col, vec3(0.5,0.5,1.0), (1.0-noteC)*(1.0-smoothstep( 0.0, 0.2, length(p-vec2(-0.8,0.0)))));

        	            fragColor = vec4( col, 1.0 );
            }
            )",
            {}
    },
    // Sound pass
    {
        {
            R"(
                // Created by inigo quilez - iq/2014
                // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

                float noise( float x )
                {    
	                return fract(sin(1371.1*x)*43758.5453);
                }

                float doString( float soundTime )
                {    
	                float noteTime = fract( soundTime/4.0 )*4.0;
	                float noteID   = floor( soundTime/4.0 );
	
	                float w = 220.0;    
	                float y = 0.0;
	
	                float t = noteTime;    
	                y += 0.5*fract( t*w*1.00 )                      *exp( -0.0010*w*t );
	                y += 0.3*fract( t*w*(1.50+0.1*mod(noteID,2.0)) )*exp( -0.0011*w*t );
	                y += 0.2*fract( t*w*2.01 )                      *exp( -0.0012*w*t );

        	                y = -1.0 + 2.0*y;
	
	                return 0.35*y;
                }

                float doChis( float soundTime )
                {
	                float inter = 0.5;
	                float noteTime = fract( soundTime/inter )*inter;
	                float noteID   = floor( soundTime/inter );
	                return 0.5*noise( noteTime )*exp(-20.0*noteTime );
                }

                float doTom( float soundTime )
                {
	                float inter = 0.5;
                    soundTime += inter*0.5;   
	
	                float noteTime = fract( soundTime/inter )*inter;
	                float f = 100.0 - 100.0*clamp(noteTime*2.0,0.0,1.0);
	                return clamp( 70.0*sin(6.2831*f*noteTime)*exp(-15.0*noteTime ), 0.0, 1.0 );
                }

                // stereo sound output. x=left, y=right
                vec2 mainSound( float time )
                {
	                vec2 y = vec2( 0.0 );
	                y += vec2(0.75,0.25) * doString( time );    
	                y += vec2(0.50,0.50) * doChis( time );    
	                y += vec2(0.50,0.50) * doTom( time );
	
	                return vec2( y );
                }
                )"
        },
        {}
    }
};

static ShadertoyTest audio_spectrum_test = {
    "Audio spectrum test.",
    //https://www.shadertoy.com/view/XdX3z2
    // Image pass
    {
        R"(
            #define bars 32.0                 // How many buckets to divide spectrum into
            #define barSize 1.0 / bars        // Constant to avoid division in main loop
            #define barGap 0.1 * barSize      // 0.1 represents gap on both sides, so a bar is
                                              // shown to be 80% as wide as the spectrum it samples
            #define sampleSize 0.02           // How accurately to sample spectrum, must be a factor of 1.0
                                              // Setting this too low may crash your browser!

                                                  // Helper for intensityToColour
            float h2rgb(float h) {
	            if(h < 0.0) h += 1.0;
	            if(h < 0.166666) return 0.1 + 4.8 * h;
	            if(h < 0.5) return 0.9;
	            if(h < 0.666666) return 0.1 + 4.8 * (0.666666 - h);
	            return 0.1;
            }

            // Map [0, 1] to rgb using hues from [240, 0] - ie blue to red
            vec3 intensityToColour(float i) {
	            // Algorithm rearranged from http://www.w3.org/TR/css3-color/#hsl-color
	            // with s = 0.8, l = 0.5
	            float h = 0.666666 - (i * 0.666666);
	
	            return vec3(h2rgb(h + 0.333333), h2rgb(h), h2rgb(h - 0.333333));
            }

            void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	            // Map input coordinate to [0, 1)
	            vec2 uv = fragCoord.xy / iResolution.xy;
	
	            // Get the starting x for this bar by rounding down
	            float barStart = floor(uv.x * bars) / bars;
	
	            // Discard pixels in the 'gap' between bars
	            if(uv.x - barStart < barGap || uv.x > barStart + barSize - barGap) {
		            fragColor = vec4(0.0);
	            }
                else
                {
	            // Sample spectrum in bar area, keep cumulative total
	            float intensity = 0.0;
	            for(float s = 0.0; s < barSize; s += barSize * sampleSize) {
		            // Shader toy shows loudness at a given frequency at (f, 0) with the same value in all channels
		            intensity += texture2D(iChannel0, vec2(barStart + s, 0.0)).r;
	            }
	            intensity *= sampleSize; // Divide total by number of samples taken (which is 1 / sampleSize)
	            intensity = clamp(intensity, 0.005, 1.0); // Show silent spectrum to be just poking out of the bottom
	
	            // Only want to keep this pixel if it is lower (screenwise) than this bar is loud
	            float i = float(intensity > uv.y); // Casting a comparison to float sets i to 0.0 or 1.0
	
	            //fragColor = vec4(intensityToColour(uv.x), 1.0);       // Demo of HSL function
	            //fragColor = vec4(i);                                  // Black and white output
	            fragColor = vec4(intensityToColour(intensity) * i, i);  // Final output
                }
	            // Note that i2c output is multiplied by i even though i is sent in the alpha channel
	            // This is because alpha is not 'pre-multiplied' for fragment shaders, you need to do it yourself
            }
            )",
            {
                { SHADERTOY_RESOURCE_MUSIC,{ "mzk00.ogg" } },
                {},
                {},
                {}
            }
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest audio_waveform_test = {
    "Audio waveform test.",
    // Image pass
    {
        R"(
            // Created by inigo quilez - iq/2013
            // License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
	            vec2 uv = -1.0+2.0*fragCoord.xy/iResolution.xy;
	            uv.x *= iResolution.x/iResolution.y;
	
	            float r = length( uv );
	            float a = atan( uv.x, uv.y );

    		            float w = texture2D( iChannel0, vec2( abs(a)/6.28,1.0) ).x;
	
	            float t = 3.0*sqrt(abs(w-0.5));

    		            float f = 0.0;
	            if( r<t ) f = (1.0-r/t);
	            vec3 col = pow( vec3(f), vec3(1.5,1.1,0.8) );

    		            fragColor = vec4( col, 1.0 );
            }
            )",
            {
                { SHADERTOY_RESOURCE_MUSIC,{ "mzk00.ogg" } },
                {},
                {},
                {}
            }
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTest audio_channel_time_test = {
    "Audio channel time test.",
    // Image pass
    {
        R"(
            const float Pi = 3.14159;
            float beat = 0.;

            void mainImage( out vec4 fragColor, in vec2 fragCoord )
            {
	            float ct = iChannelTime[0];
	            if ((ct > 8.0 && ct < 33.5)
	            || (ct > 38.0 && ct < 88.5)
	            || (ct > 93.0 && ct < 194.5))
		            beat = pow(sin(ct*3.1416*3.78+1.9)*0.5+0.5,15.0)*0.1;

    		            float scale = iResolution.y / 50.0;
	            float ring = 20.0;
	            float radius = iResolution.x*1.0;
	            float gap = scale*.5;
	            vec2 pos = fragCoord.xy - iResolution.xy*.5;
	
	            float d = length(pos);
	
	            // Create the wiggle
	            d += beat*2.0*(sin(pos.y*0.25/scale+iGlobalTime*cos(ct))*sin(pos.x*0.25/scale+iGlobalTime*.5*cos(ct)))*scale*5.0;
	
	            // Compute the distance to the closest ring
	            float v = mod(d + radius/(ring*2.0), radius/ring);
	            v = abs(v - radius/(ring*2.0));
	
	            v = clamp(v-gap, 0.0, 1.0);
	
	            d /= radius;
	            vec3 m = fract((d-1.0)*vec3(ring*-.5, -ring, ring*.25)*0.5);
	
	            fragColor = vec4(m*v, 1.0);
            }
            )",
            {
                { SHADERTOY_RESOURCE_MUSIC,{ "mzk02.ogg" } },
                {},
                {},
                {}
            }
    },
    // Sound pass
    {
        {},
        {}
    }
};

static ShadertoyTestInfo test_info;
static ShadertoyTest all_test[] = {
    mouse_test,
    resolution_test,
    textures_test,
    cubemap_test,
    date_test,
    channel_resolution_test,
    keyboard_test,
    derivatives_test,
    sound_test,
    audio_spectrum_test,
    audio_waveform_test,
    audio_channel_time_test,

};

ShadertoyTestInfo* GetTestSuite() {
    test_info.count = sizeof(all_test) / sizeof(ShadertoyTest);
    test_info.test = all_test;
    return &test_info;
}

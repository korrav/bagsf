/************************************************/
//VectorOperations.h
/************************************************/
#ifndef VECTOR_OPERATIONS_H
#define VECTOR_OPERATIONS_H
	
#include "valarray"
#include "cmath"
#include "string"
#include "TData.h"//нужен только для const fp pi = 3.1415926535897932384626433832795;

static const double ZeroVector3[3] = {0,0,0};
static const double ZeroMatrix3x3[3][3] = {{0,0,0},{0,0,0},{0,0,0}};

class TMatrix3x3;

//Класс вектора
class TVector3 {
private:
	double v[3];
	friend TVector3 operator * (const TMatrix3x3 &m, const TVector3 &v);
	friend TVector3 operator * (const fp C, const TVector3 &v);
	friend TVector3 operator + (const TVector3 &v1, const TVector3 &v2);
	friend TVector3 operator - (const TVector3 &v1, const TVector3 &v2);
	friend class TMatrix3x3;
public:
	TVector3( const double InitialVector[3] = ZeroVector3)
	{
		for (int i = 0; i <= 2; i++) v[i] = InitialVector[i];
	};

	~TVector3(){};

	void Set(double Value[3])
	{
		for (int i = 0; i <= 2; i++) v[i] = Value[i];
	};

	void Set(double x, double y, double z)
	{
		v[0] = x;
		v[1] = y;
		v[2] = z;
	};

	double operator [] (int idx) {return v[idx];};
	
	void operator = (TVector3 &PV)	{
		for (int i = 0; i <= 2; i++) this->v[i] = PV.v[i];
	};
	void operator = (const TVector3 &PV)	{
		for (int i = 0; i <= 2; i++) this->v[i] = PV.v[i];
	};
};

//Класс матрицы
class TMatrix3x3 {
private:
	TVector3 v[3];
	friend TVector3 operator * (const TMatrix3x3 &m, const TVector3 &v);
public:
	TMatrix3x3(const double InitialMatrix[3][3] = ZeroMatrix3x3)
	{
		for (int i = 0; i <= 2; i++)
			for (int j = 0; j <= 2; j++) {v[i].v[j] = InitialMatrix[i][j];}
	};
	~TMatrix3x3(){};

	void Set(double Value[3][3])
	{
		for (int i = 0; i <= 2; i++)
			for (int j = 0; j <= 2; j++) {v[i].v[j] = Value[i][j];}
	};
};

//Операция умножения матрицы на вектор
inline TVector3 operator * (const TMatrix3x3 &m, const TVector3 &v)
{
	TVector3 r;
	for (int i = 0; i <= 2; i++) {  
			r.v[i] = 0;
			for ( int j = 0; j <= 2; j++)
				r.v[i] += m.v[i].v[j] * v.v[j];
	}
	return r;
};

//Операция умножения числа на вектор
inline TVector3 operator * (const fp C, const TVector3 &v)
{
	TVector3 r;
	for (int i = 0; i <= 2; i++) {  
		r.v[i] = v.v[i] * C;
	}
	return r;
};

//Операция сложения векторов
inline TVector3 operator + (const TVector3 &v1, const TVector3 &v2)
{
	TVector3 r;
	for (int i = 0; i<=2; i++) r.v[i] = v1.v[i] + v2.v[i];
	return r;
};

//Операция вычитания векторов
inline TVector3 operator - (const TVector3 &v1, const TVector3 &v2)
{
	TVector3 r;
	for (int i = 0; i<=2; i++) r.v[i] = v1.v[i] - v2.v[i];
	return r;
};

inline TVector3 EulerRotate(const TVector3 &v, const double Thet, const double Phi, const double Alp)
{
	TVector3 r;
	const double SinT = sin(Thet);
	const double CosT = cos(Thet);
	const double SinP = sin(Phi);
	const double CosP = cos(Phi);
	const double SinA = sin(Alp);
	const double CosA = cos(Alp);

	const double Rr[3][3] = {{CosA,-SinA,0},{SinA,CosA,0},{0,0,1}};
	const double Ry[3][3] = {{CosP,0,SinP},{0,1,0},{-SinP,0,CosP}};
	const double RyOb[3][3] = {{CosP,0,-SinP},{0,1,0},{SinP,0,CosP}};
	const double Rz[3][3] = {{CosT,-SinT,0},{SinT,CosT,0},{0,0,1}};
	const double RzOb[3][3] = {{CosT,SinT,0},{-SinT,CosT,0},{0,0,1}};

	TMatrix3x3 rr(Rr);
	TMatrix3x3 ry(Ry);
	TMatrix3x3 ryob(RyOb);
	TMatrix3x3 rz(Rz);
	TMatrix3x3 rzob(RzOb);

	r = rz*(ry*(rr*(ryob*(rzob*v))));
	return r;
};

//Расстояние между двумя точками
inline fp D(TVector3 &v1, TVector3 &v2)
{
	return sqrt(pow(v1[0]-v2[0],2.) + pow(v1[1]-v2[1],2.) + pow(v1[2]-v2[2],2.));
};

//Направляющие углы Phi,Thet из точки v1 в точку v2
inline void GetDirection (TVector3 &v1, TVector3 &v2, fp &Phi, fp &Thet)
{
	fp X = v2[0] - v1[0];
	fp Y = v2[1] - v1[1];
	if (X < 0) Phi = pi + atan( Y / X );
	else if (Y >= 0) Phi = atan( Y / X ); else Phi = 2*pi + atan( Y / X ); 
	Thet = acos( (v2[2] - v1[2]) / D(v1,v2) );
};

/************************************************/
//Далее действия с векторами на основе valarray

//Вывод матрицы
inline string print_matrix (const FA & vec, int m, int n, char *mname) //fityk
    //m rows, n columns
{ 
    /*if (F->get_verbosity() <= 0)  //optimization (?)
        return "";*/
    /*assert (size(vec) == m * n);*/
    if (m < 1 || n < 1)
        cout<< "In `print_matrix': It is not a matrix.";
	/*ostringstream h;*/
    cout << mname << "={ ";
    if (m == 1) { // vector 
        for (int i = 0; i < n; i++)
            cout << vec[i] << (i < n - 1 ? ", " : " }") ;
    }
    else { //matrix 
        std::string blanks (strlen (mname) + 1, ' ');
        for (int j = 0; j < m; j++){
            if (j > 0)
                cout << blanks << "  ";
            for (int i = 0; i < n; i++) 
                cout << vec[j * n + i] << ", ";
            cout << endl;
        }
        cout << blanks << "}";
    }
    return "  ";/*h.str();*/
}//end of print_matrix

// This function solves a set of linear algebraic equations using
// Jordan elimination with partial pivoting.
//
// A * x = b
// 
// A is n x n matrix, fp A[n*n]
// b is vector b[n],   
// Function returns vector x[] in b[], and 1-matrix in A[].
// return value: true=OK, false=singular matrix
//   with special exception: 
//     if i'th row, i'th column and i'th element in b all contains zeros,
//     it's just ignored, 

inline void Jordan(FA &A, FA &b, int n) //fityk
{//var
	int i,j;
//begin
	/*assert (size(A) == n*n && size(b) == n);*/
    for (/*int*/ i = 0; i < n; i++) {
        fp amax = 0;                    // looking for a pivot element
        int maxnr = -1;  
        for (/*int*/ j = i; j < n; j++)                     
            if (fabs (A[n*j+i]) > amax) {
                maxnr = j;
                amax = fabs (A[n * j + i]);
            }
        if (maxnr == -1) {    // singular matrix
            // i-th column has only zeros. 
            // If it's the same about i-th row, and b[i]==0, let x[i]==0. 
            /*for (int j = i; j < n; j++)
                if (A[n * i + j] || b[i]) {
                    //F->vmsg (print_matrix(A, n, n, "A"));
                    F->msg (print_matrix(b, 1, n, "b"));

                    throw ExecuteError("In iteration " + S(iter_nr)
                                       + ": trying to reverse singular matrix."
                                        " Column " + S(i) + " is zeroed.");
                }*/
            continue; // x[i]=b[i], b[i]==0
        }
        if (maxnr != i) {                            // interchanging rows
            for (int j=i; j<n; j++)
                swap (A[n*maxnr+j], A[n*i+j]);
            swap (b[i], b[maxnr]);
        }
        register fp foo = 1.0 / A[i*n+i];
		for (int j = i; j < n; j++)
            A[i*n+j] *= foo;
		b[i] *= foo;
        for (int k = 0; k < n; k++)
            if (k != i) {
                foo = A[k * n + i];
                for (int j = i; j < n; j++)
                    A[k * n + j] -= A[i * n + j] * foo;
                b[k] -= b[i] * foo;
            }
    }
}//end of Jordan

/// A - matrix n x n; returns A^(-1) in A
inline void reverse_matrix (FA & A, int n) //fityk
{
    //no need to optimize it
    /*assert (size(A) == n*n);*/
    FA A_result(n*n);   
    for (int i = 0; i < n; i++) {
        FA A_copy = A;      
        FA v(n);/*,0);*/
        v = 0;
		v[i] = 1;
        Jordan(A_copy, v, n);
		for (int j = 0; j < n; j++) 
            A_result[j * n + i] = v[j];
    }
    A = A_result;
}//end of reverse_matrix


//Перемножает 2 матрицы или вектор и матрицу ma==nb!!!
//!!! Переписать шаблоном
inline FA mm (FA * A, FA * B, int na, int manb, int mb)
{//var
	FA result(na*mb);
	int imm,jmm;	
//begin
	for (imm = 0; imm < na; imm++) 
		for (jmm = 0; jmm < mb; jmm++)
			result[jmm*na+imm] = ( FA((*A)[slice(imm,manb,na)]) * FA((*B)[slice(jmm*manb,manb,1)]) ).sum();
	return(result);
}//end of mm

inline CA mm (CA * A, CA * B, int na, int manb, int mb)
{//var
	CA result(na*mb);
	int imm,jmm;	
//begin
	for (imm = 0; imm < na; imm++) 
		for (jmm = 0; jmm < mb; jmm++)
			result[jmm*na+imm] = ( CA((*A)[slice(imm,manb,na)]) * CA((*B)[slice(jmm*manb,manb,1)]) ).sum();
	return(result);
}//end of mm


//Транспонирует матрицу,пергружаемая функция заменить шаблоном
inline FA Transpose (FA * A, int n, int m)
{//var
	FA result(n*m); 
	int it;
//begin
	for (it = 0; it < m; it++)
		result[slice(it,n,m)] = FA( (*A)[slice(it*n,n,1)] );
	return(result);
}//end of Transpose



//Тип указатель на аппроксимирующую функцию 
//X - независимые значения
//params - подбираемые параметры функции
//f - зависимые значения
//pder - производные по параметрам
typedef void (*TApproximatingFunction)(FA * /*X*/, FA * /*params*/, FA * /*f*/, FA * /*pder*/);

//IDL "CurveFit"
//       Non-linear least squares fit to a function of an arbitrary
//       number of parameters.  The function may be any non-linear
//       function.  If available, partial derivatives can be calculated by
//       the user function, else this routine will estimate partial derivatives
//       with a forward difference approximation.
//       Copied from "CURFIT", least squares fit to a non-linear
//       function, pages 237-239, Bevington, Data Reduction and Error
//       Analysis for the Physical Sciences.  This is adapted from:
//       Marquardt, "An Algorithm for Least-Squares Estimation of Nonlinear
//       Parameters", J. Soc. Ind. Appl. Math., Vol 11, no. 2, pp. 431-441,
//       June, 1963.
//
// CALLING SEQUENCE:
//       Result = CURVEFIT(X, Y, Weights, A, SIGMA, FUNCTION_NAME = name, $
//                         ITMAX=ITMAX, ITER=ITER, TOL=TOL, /NODERIVATIVE)
//
// INPUTS:
//       X:  A row vector of independent variables.  This routine does
//           not manipulate or use values in X, it simply passes X
//           to the user-written function.
//
//       Y:  A row vector containing the dependent variable.
//
//  Weights:  A row vector of weights, the same length as Y.
//            For no weighting,
//                 Weights(i) = 1.0.
//            For instrumental (Gaussian) weighting,
//                 Weights(i)=1.0/sigma(i)^2
//            For statistical (Poisson)  weighting,
//                 Weights(i) = 1.0/y(i), etc.
//
//       A:  A vector, with as many elements as the number of terms, that
//           contains the initial estimate for each parameter.  IF A is double-
//           precision, calculations are performed in double precision,
//           otherwise they are performed in single precision. Fitted parameters
//           are returned in A.
inline FA Fit (FA * X, FA * Y, FA * Weights, FA * a, fp * chisq, TApproximatingFunction AF )
{	
	//ПРОВЕРЯТЬ СООТВЕТСТВИЕ ДЛИН ВХОДНЫХ МАССИВОВ, ВЫКИНУТЬ ЭКСЕПШН
	//ОПТИМИЗИРОВАТЬ МАТРИЧНЫЕ ВЫЧИСЛЕНИЯ

	const fp tol = 0.0001;
	const int itmax = 15;	
	const fp flambdafactor = 100;//1./100.;//10
	const int maxlambdacount = 10;//10;//30
	const fp flambda0 = 1e-10;//1.e6;//0.0001

	fp	flambda,
			chisq1;
	
	int	nterms = a->size(),
			ny = Y->size(),
			nfree = ny-nterms,
			fi;
	
	FA	pder, 
			yfit, 
			beta, 
			temp, 
			temp2, 
			alpha, 
			sigma,
			sigma1, 
			yfit1, 
			c, 
			array, 
			b;
	
	valarray <size_t> diag(nterms);
	
	for (fi = 0; fi < nterms; fi++) diag[fi] = fi*(nterms+1); //������ ������������ ���������
	pder.resize(ny*nterms);

	for (int iter = 0; iter < itmax; iter++)	{	
		// The user's procedure will return partial derivatives
		(*AF)(X, a, &yfit, &pder);
		
		// Evaluate alpha and beta matricies.
		temp = (*Y-yfit) * (*Weights);
		beta = mm(&temp, &pder, 1, ny, nterms); //1*nterms

		temp.resize(nterms); 
		temp = 1.;
		temp = mm(Weights, &temp, ny, 1, nterms) * pder;
		temp2 = Transpose(&pder, ny, nterms);
		alpha = mm(&temp2, &temp, nterms, ny, nterms);  //nterms*nterms

		//   save current values of return parameters
		sigma1 = sqrt( 1. / FA (alpha[diag]) ); //Current sigma.1*nterms
		sigma  = sigma1;

 		// Current chi squared.scalar
		chisq1 = (( pow(*Y - yfit, 2.) * (*Weights) ).sum()) / fp(nfree);
    *chisq = chisq1;

    yfit1 = yfit;

		if (chisq1 < ((abs(*Y)).sum())/(fp(nfree))) break;

		c = sqrt(FA(alpha[diag]));
		c = mm(&c, &c, nterms, 1, nterms);  //nterms*nterms

		int lambdacount = 0;
		flambda = flambda0;
		do {
			lambdacount++;
			// Normalize alpha to have unit diagonal.
      array = alpha / c; //nterms*nterms
			array[diag] = FA(array[diag]) * (1 + flambda);
		    
			// Invert modified curvature matrix to find new parameters.
			reverse_matrix(array, nterms);
			temp = array/c; 
			b = mm(&temp, &beta, nterms, nterms, 1); // New params
			(*AF)(X, &b, &yfit, &pder);
			*chisq = (( pow(*Y - yfit, 2.) * (*Weights) ).sum()) / fp(nfree);// New chisq
			sigma = sqrt(FA(array[diag]) / FA(alpha[diag]) );// New sigma
			//if (!finite(chisq) || (lambdacount >= maxlambdacount) && (chisq >= chisq1)) {
			if (lambdacount >= maxlambdacount) {
        // Reject changes made this iteration, use old values.
        yfit  = yfit1;
        sigma = sigma1;
        *chisq = chisq1;
			};
			flambda *= flambdafactor;
		} while ((*chisq >= chisq1) && (lambdacount < maxlambdacount));//> chisq1);
		
		//flambda /= 100;
    *a=b;// Save new parameter estimate.
		if (((chisq1-*chisq)/chisq1) <= tol) break;  //Finished?
	
	};	//for

	//chi2 = chisq;         // Return chi-squared (chi2 obsolete-still works)
    //if done_early THEN iter = iter - 1
	return(yfit);
};	//end of Fit

#endif

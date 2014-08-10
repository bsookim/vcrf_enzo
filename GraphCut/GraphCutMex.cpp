#include "mex.h"
#include "GCoptimization.h"
#include "GraphCut.h"
#include <stdlib.h>

/* Declarations */
void SetLabels(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void SetDetLabels(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void SetDetFixBool(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]); // Msun added 9/13
void GetLabels(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void ABswaps(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void Expand(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void DetClassEnergy(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void DetClassEnergyXY(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void DetClassEnergyPrior(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void Energy(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
GCoptimization* GetGCHandle(const mxArray *x);    /* extract ahndle from mxArry */


// test comments erro

/*
 * Matlab wrapper for Weksler graph cut implementation
 * 
 * GCoptimization cde by Olga Veksler.
 * Wrapper code by Shai Bagon.
 *
 *
 *   Performing Graph Cut operations on a 2D grid.
 *   
 *   Usage:
 *       [gch ...] = GraphCutMex(gch, mode ...);
 *   
 *   Notes:
 *   1. Data types are crucial!
 *   2. The embedded implementation treat matrices as a row stack, while
 *       matlab treats them as column stack; thus there is a need to
 *       transpose the label matrices and the indices passed to expand
 *       algorithm.
 *
 *   Inputs:
 *   - gch: a valid Graph Cut handle (use GraphCutConstr to create a handle).
 *   - mode: a char specifying mode of operation. See details below.
 *
 *   Output:
 *   - gch: A handle to the constructed graph. Handle this handle with care
 *              and don't forget to close it in the end!
 *
 *   Possible modes:
 *
 *   - 's': Set labels
 *           [gch] = GraphCutMex(gch, 's', labels)
 *
 *       Inputs:
 *           - labels: a width*height array of type int32, containing a
 *              label per pixel. note that labels values must be is range
 *              [0..num_labels-1]. 
 *
 *   - 'g': Get current labeling
 *           [gch labels] = GraphCutMex(gch, 'g')
 *
 *       Outputs:
 *           - labels: a width*height array of type int32, containing a
 *              label per pixel. note that labels values must be is range
 *              [0..num_labels-1].
 *
 *   - 'n': Get current values of energy terms
 *           [gch se de] = GraphCutMex(gch, 'n')
 *
 *       Outputs:
 *           - se: Smoothness energy term.
 *           - de: Data energy term.
 *
 *   - 'e': Perform labels expansion
 *           [gch labels] = GraphCutMex(gch, 'e')
 *           [gch labels] = GraphCutMex(gch, 'e', iter)
 *           [gch labels] = GraphCutMex(gch, 'e', [], label)
 *           [gch labels] = GraphCutMex(gch, 'e', [], label, indices)
 *
 *       When no inputs are provided, GraphCut performs expansion steps
 *       until it converges.
 *
 *       Inputs:
 *           - iter: a double scalar, the maximum number of expand
 *                      iterations to perform.
 *           - label: int32 scalar denoting the label for which to perfom
 *                        expand step.
 *           - indices: int32 array of linear indices of pixels for which
 *                            expand step is computed. indices _MUST_ be zero offset and not
 *                            one offset like matlab usuall indexing system, i.e., to include
 *                            the first pixel in the expand indices array must contain 0 and
 *                            NOT 1!
 *
 *       Outputs:
 *           - labels: a width*height array of type int32, containing a
 *              label per pixel. note that labels values must be is range
 *              [0..num_labels-1].
 *
 *   - 'a': Perform alpha - beta swappings
 *           [gch labels] = GraphCutMex(gch, 'a')
 *           [gch labels] = GraphCutMex(gch, 'a', iter)
 *           [gch labels] = GraphCutMex(gch, 'a', label1, label2)
 *
 *       When no inputs are provided, GraphCut performs alpha - beta swaps steps
 *       until it converges.
 *
 *       Inputs:
 *           - iter: a double scalar, the maximum number of swap
 *                      iterations to perform.
 *           - label1, label2: int32 scalars denoting two labels for swap
 *                                       step.
 *
 *       Outputs:
 *           - labels: a width*height array of type int32, containing a
 *              label per pixel. note that labels values must be is range
 *              [0..num_labels-1].
 *
 *   - 'c': Close the graph and release allocated resources.
 *       [gch] = GraphCutMex(gch,'c');
 *
 *
 *   See Also:
 *       GraphCutConstr
 *
 *   This wrapper for Matlab was written by Shai Bagon (shai.bagon@weizmann.ac.il).
 *   Department of Computer Science and Applied Mathmatics
 *   Wiezmann Institute of Science
 *   http://www.wisdom.weizmann.ac.il/
 *
 *   The core cpp application was written by Veksler Olga:
 * 	  
 *   [1] Efficient Approximate Energy Minimization via Graph Cuts
 *       Yuri Boykov, Olga Veksler, Ramin Zabih,
 *       IEEE transactions on PAMI, vol. 20, no. 12, p. 1222-1239, November 2001.
 * 
 *   [2] What Energy Functions can be Minimized via Graph Cuts?
 *       Vladimir Kolmogorov and Ramin Zabih.
 *       To appear in IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI).
 *       Earlier version appeared in European Conference on Computer Vision (ECCV), May 2002.
 * 
 *   [3] An Experimental Comparison of Min-Cut/Max-Flow Algorithms
 *       for Energy Minimization in Vision.
 *       Yuri Boykov and Vladimir Kolmogorov.
 *       In IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI),
 *       September 2004
 *
 *   [4] Matlab Wrapper for Graph Cut.
 *       Shai Bagon.
 *       in www.wisdom.weizmann.ac.il/~bagon, December 2006.
 *     
 * 	This software can be used only for research purposes, you should cite
 * 	the aforementioned papers in any resulting publication.
 * 	If you wish to use this software (or the algorithms described in the aforementioned paper)
 * 	for commercial purposes, you should be aware that there is a US patent: 
 * 
 * 		R. Zabih, Y. Boykov, O. Veksler, 
 * 		"System and method for fast approximate energy minimization via graph cuts ", 
 * 		United Stated Patent 6,744,923, June 1, 2004
 *
 *
 *   The Software is provided "as is", without warranty of any kind.
 *
 *
 */
void mexFunction(
    int		  nlhs, 	/* number of expected outputs */
    mxArray	  *plhs[],	/* mxArray output pointer array */
    int		  nrhs, 	/* number of inputs */
    const mxArray	  *prhs[]	/* mxArray input pointer array */
    )
{
    /* building graph is only call without gch */
    GCoptimization *MyGraph = NULL;
    char mode ='\0';
    if ( nrhs < 2 ) {
        mexErrMsgIdAndTxt("GraphCut","Too few input arguments");
    }

    /* get graph handle */
    MyGraph = GetGCHandle(prhs[0]);
    
    /* get mode */
    GetScalar(prhs[1], mode);
    
   
    switch (mode) {
        case 'c': /* close */
            delete MyGraph; /* ->~GCoptimization(); /* explicitly call the destructor */
            MyGraph = NULL;
            break;
        case 's': /* set labels */
            /* setting the labels: we have an extra parameter - int32 array of size w*l
             * containing the new labels */
            SetLabels(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
		case 'f':
			SetDetFixBool(MyGraph, nlhs, plhs, nrhs, prhs);
			break;
        case 'y': /* set thing indicator labels */
            /* setting the labels: we have an extra parameter - int32 array of size w*l
             * containing the new labels */
            SetDetLabels(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
        case 'g': /* get labels */
            GetLabels(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
        case 'a': /* a-b swaps */
            ABswaps(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
        case 'e': /* expand steps */
            Expand(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
		// <!--Msun
        //case 'q': /* expand_QPBO steps */
		//	MyGraph->UseQPBO(); // set to use QPBO solver
		//	MyGraph->m_hopType = 1; // set to use QPBO solver
        //    Expand(MyGraph, nlhs, plhs, nrhs, prhs);
        //    break;
        //case 'd': /* expand_QPBO det formulation */
		//	MyGraph->UseQPBO(); // set to use QPBO solver
		//	MyGraph->m_hopType = 0; // set to use QPBO solver
        //    Expand(MyGraph, nlhs, plhs, nrhs, prhs);
        //    break;
		case 'd':
			DetClassEnergy(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
		case 'x':
			DetClassEnergyXY(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
		case 'p':
			DetClassEnergyPrior(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
		// Msun -->
        case 'n': /* get the current labeling energy */
            Energy(MyGraph, nlhs, plhs, nrhs, prhs);
            break;
        default:
            mexErrMsgIdAndTxt("GraphCut:mode","unrecognized mode");
    }
    /* update the gch output handle */
    GraphHandle *pgh;
    plhs[0] = mxCreateNumericMatrix(1, 1, MATLAB_POINTER_TYPE, mxREAL);
    pgh = (GraphHandle*) mxGetData(plhs[0]);
    *pgh = (GraphHandle)(MyGraph);
}
/**************************************************************************************/
/* set user defined labels to graph */
void SetLabels(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    /* we need exactly 3 input arguments: gch, mode, labels */
    if (nrhs != 3 ) 
        mexErrMsgIdAndTxt("GraphCut:SetLabels","Wrong number of input arguments");
    if ( mxGetClassID(prhs[2]) != mxINT32_CLASS ) 
        mexErrMsgIdAndTxt("GraphCut:SetLabels","labels array not int32");
    if ( mxGetNumberOfElements(prhs[2]) != MyGraph->GetNumPixels() )
        mexErrMsgIdAndTxt("GraphCut:SetLabels","wrong number of elements in labels array");
    
    /* verify only one output parameter */
    if (nlhs != 1)
        mexErrMsgIdAndTxt("GraphCut:SetLabels","wrong number of output parameters");
    
    MyGraph->SetAllLabels( (GCoptimization::LabelType*) mxGetData(prhs[2]) );
}
/**************************************************************************************/
/* set user defined labels to graph */
void SetDetLabels(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    /* we need exactly 3 input arguments: gch, mode, labels */
    if (nrhs != 3 ) 
        mexErrMsgIdAndTxt("GraphCut:SetDetLabels","Wrong number of input arguments");
    if ( mxGetClassID(prhs[2]) != mxLOGICAL_CLASS ) 
        mexErrMsgIdAndTxt("GraphCut:SetDetLabels","labels array not logic");
    if ( mxGetNumberOfElements(prhs[2]) != MyGraph->GetnHigherDet() )
        mexErrMsgIdAndTxt("GraphCut:SetDetLabels","wrong number of elements in labels array");
    
    /* verify only one output parameter */
    if (nlhs != 1)
        mexErrMsgIdAndTxt("GraphCut:SetDetLabels","wrong number of output parameters");
    
    MyGraph->SetDetBoolLabels((const bool*) mxGetData(prhs[2]) );
}
/**************************************************************************************/
/* set user defined labels to graph */
void SetDetFixBool(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    /* we need exactly 3 input arguments: gch, mode, labels */
    if (nrhs != 3 ) 
        mexErrMsgIdAndTxt("GraphCut:SetDetFixBool","Wrong number of input arguments");
    if ( mxGetClassID(prhs[2]) != mxLOGICAL_CLASS ) 
        mexErrMsgIdAndTxt("GraphCut:SetDetFixBool","labels array not logic");
    if ( mxGetNumberOfElements(prhs[2]) != MyGraph->GetnHigherDet() )
        mexErrMsgIdAndTxt("GraphCut:SetDetFixBool","wrong number of elements in labels array");
    
    /* verify only one output parameter */
    if (nlhs != 1)
        mexErrMsgIdAndTxt("GraphCut:SetDetFixBool","wrong number of output parameters");
    
    MyGraph->SetDetFixBool((const bool*) mxGetData(prhs[2]) );
}
/**************************************************************************************/
/* set user defined labels to graph */
void GetLabels(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    /* exactly two input arguments */
    if ( nrhs != 2 ) 
        mexErrMsgIdAndTxt("GraphCut:GetLabels","Wrong number of input arguments");
        
    /* we need exactly 2 output arguments: gch, labels */
    if (nlhs != 2 ) 
        mexErrMsgIdAndTxt("GraphCut:GetLabels","Wrong number of output arguments");

    /* transposed result */
    plhs[1] = mxCreateNumericMatrix(MyGraph->GetWidth(), MyGraph->GetNumPixels() / MyGraph->GetWidth(), mxINT32_CLASS, mxREAL);
    MyGraph->ExportLabels( (GCoptimization::LabelType*) mxGetData(plhs[1]) );
}
/**************************************************************************************/
/* support several kinds of a/b swaps:
 * 1. defualt - swap till convergence (no extra parameters)
 * 2. #iterations - performs #iterations:   GraphCut(gch, mode, #iteration)
 * 3. swap two labels - GraphCut(gch, mode, l1, l2)
 */
void ABswaps(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    int max_iterations(0), li(0);
    GCoptimization::LabelType label[2] = { 0, 1};
    GCoptimization::PixelType *indices = NULL;
    
    /* check inputs */
    switch (nrhs) {
        case 2:
            /* default - expand till convergence */
            MyGraph->swap();
            break;
        case 3:
            /* number of iterations */
            GetScalar(prhs[2], max_iterations);
            if ( max_iterations == 1 ) 
                MyGraph->oneSwapIteration();
            else
                MyGraph->swap(max_iterations);
            break;
        case 4:
            for ( li = 2; li<4;li++) {
                GetScalar(prhs[li], label[li-2]);
                if ( label[li-2] < 0 || label[li-2] >= MyGraph->GetNumLabels() ) 
                    mexErrMsgIdAndTxt("GraphCut:swap","invalid label value");
            }
            MyGraph->alpha_beta_swap(label[0], label[1]);
            break;
        default:
            mexErrMsgIdAndTxt("GraphCut:swap","Too many input arguments");
    }
        
    /* output: at least one (gch) can output the labels as well */
    if ( nlhs > 2 ) 
        mexErrMsgIdAndTxt("GraphCut:swap","too many output arguments");

    /* output labels */
    if ( nlhs >= 2 ) {
        /* set the lables */
        
        /* transposed result */
        plhs[1] = mxCreateNumericMatrix(MyGraph->GetWidth(), MyGraph->GetNumPixels() / MyGraph->GetWidth(), mxINT32_CLASS, mxREAL);
        MyGraph->ExportLabels( (GCoptimization::LabelType*) mxGetData(plhs[1]) );
    }
                    
}

/**************************************************************************************/
/* support several kinds of expansion 
 * 1. default - expand untill convergence no extra parameters
 * 2. #iterations - performs #iterations:   GraphCut(gch, mode, #iteration)
 * 3. expand label - expand specific label  GraphCut(gch, mode, [], label)
 * 4. expand label at specific indices      GraphCut(gch, mode, [], label, indices) indices start with zero and not 1 like usuall matlab inices!!
 */
void Expand(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{   
    int num(0), max_iterations(0);
	int* before_exp;
    GCoptimization::LabelType label(0);
    GCoptimization::PixelType *indices = NULL;
   
	// Msun: make no randomness for label
#ifdef RAND_COMPILE 
	MyGraph->setLabelOrder(true);
#else
	MyGraph->setLabelOrder(false);
#endif
 
	// Msun: make no randomness for det bool
#ifdef RAND_COMPILE 
	MyGraph->setPairThingOrder(true);
#else
	MyGraph->setPairThingOrder(false);
#endif

    /* check inputs */
    switch (nrhs) {
        case 2:
            /* default - expand till convergence */
			// debug
			before_exp = new int[MyGraph->GetnHigherDet()];
			MyGraph->ExportDetLabels(before_exp);


            MyGraph->expansion();
#ifdef MEX_COMPILE
#ifdef LOG_ON
			mexPrintf("BBBBBefore expansion: \n");
			for(int i=0;i<MyGraph->GetnHigherDet();i++) {
				mexPrintf("%d ", before_exp[i]);
			}
			
			mexPrintf("\nAAAAAfter expansion: \n");
#endif
#endif
			MyGraph->printThing();
			delete [] before_exp;
            break;
        case 3:
            /* number of iterations */
            GetScalar(prhs[2], max_iterations);
            if ( max_iterations == 1 ) 
                MyGraph->oneExpansionIteration();
            else
                MyGraph->expansion(max_iterations);
            break;
        case 5:
            /* get indices */
            if (mxGetClassID(prhs[4]) != mxINT32_CLASS)
                mexErrMsgIdAndTxt("GraphCut:Expand","indices must be int32");
            num = mxGetNumberOfElements(prhs[4]);
            if (num < 0 || num > MyGraph->GetNumPixels())
                mexErrMsgIdAndTxt("GraphCut:Expand","too many indices");
            indices = (GCoptimization::PixelType*)mxGetData(prhs[4]);
        case 4:
            /* expand specific label */
            if (mxGetNumberOfElements(prhs[2]) != 0)
                mexErrMsgIdAndTxt("GraphCut:Expand","third argument must empty");
            GetScalar(prhs[3], label);

            if ( indices != NULL ) {
                /* expand specific label at specific indices */
                MyGraph->alpha_expansion(label, indices, num); // Msun: ??? where to specify indices???
            } else
                MyGraph->alpha_expansion(label);

            break;
        default:
            mexErrMsgIdAndTxt("GraphCut:Expand","Too many input arguments");
    }
        
    /* output: at least one (gch) can output the labels as well */
    if ( nlhs > 3 ) 
        mexErrMsgIdAndTxt("GraphCut:Expand","too many output arguments");

    /* output element labels */
    if ( nlhs >= 2 ) {

        /* transposed result */        
        plhs[1] = mxCreateNumericMatrix(MyGraph->GetWidth(), MyGraph->GetNumPixels() / MyGraph->GetWidth(), mxINT32_CLASS, mxREAL);
        MyGraph->ExportLabels( (GCoptimization::LabelType*) mxGetData(plhs[1]) );

    }

    /* output indicator labels */
    if ( nlhs >= 3 ) {
        
        /* transposed result */        
        plhs[2] = mxCreateNumericMatrix( MyGraph->GetnHigherDet(), 1, mxINT32_CLASS, mxREAL);
        MyGraph->ExportDetLabels( (GCoptimization::LabelType*) mxGetData(plhs[2]) );

    }
}

/**************************************************************************************/

void DetClassEnergy(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if ( nrhs != 3 )
        mexErrMsgIdAndTxt("GraphCut:Energy","Too many input arguments");
    if ( nlhs < 1 )
        mexErrMsgIdAndTxt("GraphCut:Energy","wrong number of output arguments");
    
    GCoptimization::EnergyType *e;
	int* class_label = (int*) mxGetData(prhs[2]);
 
    plhs[1] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
    e = (GCoptimization::EnergyType *)mxGetData(plhs[1]);
#ifdef OLD_DET_COMPILE		
	*e = MyGraph->giveClassDetHOPEnergy(class_label[0]);
#else
	*e = MyGraph->giveClassDetHOP2Energy(class_label[0]);
	//mexPrintf("used giveClassDetHOP2Energy\n");
#endif
}

void DetClassEnergyXY(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if ( nrhs != 3 )
        mexErrMsgIdAndTxt("GraphCut:Energy","Too many input arguments");
    if ( nlhs < 1 )
        mexErrMsgIdAndTxt("GraphCut:Energy","wrong number of output arguments");
    
    GCoptimization::EnergyType *e;
	int* class_label = (int*) mxGetData(prhs[2]);
 
    plhs[1] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
    e = (GCoptimization::EnergyType *)mxGetData(plhs[1]);
#ifdef OLD_DET_COMPILE		
	*e = MyGraph->giveClassDetHOPEnergy(class_label[0]);
#else
	*e = MyGraph->giveClassDetHOP2XYEnergy(class_label[0]);
	//mexPrintf("used giveClassDetHOP2Energy\n");
#endif
}

void DetClassEnergyPrior(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if ( nrhs != 3 )
        mexErrMsgIdAndTxt("GraphCut:Energy","Too many input arguments");
    if ( nlhs < 1 )
        mexErrMsgIdAndTxt("GraphCut:Energy","wrong number of output arguments");
    
    GCoptimization::EnergyType *e;
	int* class_label = (int*) mxGetData(prhs[2]);
 
    plhs[1] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
    e = (GCoptimization::EnergyType *)mxGetData(plhs[1]);
#ifdef OLD_DET_COMPILE		
	*e = MyGraph->giveClassDetHOPEnergy(class_label[0]);
#else
	*e = MyGraph->giveClassDetHOP2PriorEnergy(class_label[0]);
	//mexPrintf("used giveClassDetHOP2Energy\n");
#endif
}
/**************************************************************************************/
/* extract energy of labeling  
 * [gch se de] = GraphCut(gch, 'n');
 */
void Energy(GCoptimization *MyGraph, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if ( nrhs != 2 )
        mexErrMsgIdAndTxt("GraphCut:Energy","Too many input arguments");
    if ( nlhs < 3 )
        mexErrMsgIdAndTxt("GraphCut:Energy","wrong number of output arguments");
    
    GCoptimization::EnergyType *e;
    
    plhs[1] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
    e = (GCoptimization::EnergyType *)mxGetData(plhs[1]);
    *e = MyGraph->giveSmoothEnergy();
    
    plhs[2] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
    e = (GCoptimization::EnergyType *)mxGetData(plhs[2]);
    *e = MyGraph->giveDataEnergy();

	if (nlhs > 3){
		plhs[3] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
		e = (GCoptimization::EnergyType *)mxGetData(plhs[3]);
		*e = MyGraph->giveHOPEnergy();
	}

	if (nlhs > 4){
		plhs[4] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
		e = (GCoptimization::EnergyType *)mxGetData(plhs[4]);
#ifdef OLD_DET_COMPILE		
		*e = MyGraph->giveDetHOPEnergy();
#else
		*e = MyGraph->giveDetHOP2Energy();
		//mexPrintf("used giveDetHOP2Energy\n");
#endif
	}

	if (nlhs > 5){
		plhs[5] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
		e = (GCoptimization::EnergyType *)mxGetData(plhs[5]);
		*e = MyGraph->giveOccHOPEnergy();
	}

	if (nlhs > 6){
		plhs[6] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
		e = (GCoptimization::EnergyType *)mxGetData(plhs[6]);
		*e = MyGraph->givePairThingEnergy();
	}

	if (nlhs > 7){
		plhs[7] = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
		e = (GCoptimization::EnergyType *)mxGetData(plhs[7]);
		if (MyGraph->m_num_comp_labels == 0){
			*e = MyGraph->giveClassCoEnergy();
		}else{
			*e = MyGraph->giveCompClassCoEnergy();
		}
	}
}
/**************************************************************************************/
GCoptimization* GetGCHandle(const mxArray *x)
{
    GraphHandle gch = 0;
    GCoptimization* MyGraph = 0;
    
    if ( mxGetNumberOfElements(x) != 1 ) {
        mexErrMsgIdAndTxt("GraphCut:handle",
        "Too many GC handles");
    }
    if ( mxGetClassID(x) != MATLAB_POINTER_TYPE ) {
        mexErrMsgIdAndTxt("GraphCut:handle",
        "GC handle argument is not of proper type");
    }
    gch = (GraphHandle*)mxGetData(x);
    MyGraph = (GCoptimization*)(*(POINTER_CAST*)gch);
    if ( MyGraph==NULL  ) {
        mexErrMsgIdAndTxt("1 GraphCut:handle",
        "GC handle is not valid");
    }
    if ( ! MyGraph->IsClassValid() ) {
        mexErrMsgIdAndTxt("2 GraphCut:handle",
        "GC handle is not valid");
    }

    
    return MyGraph;
}
/**************************************************************************************/

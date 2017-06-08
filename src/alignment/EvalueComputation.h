#ifndef MMSEQS_EVALUE_COMPUTATION_H
#define MMSEQS_EVALUE_COMPUTATION_H

#include "SubstitutionMatrix.h"
#include "Util.h"
#include "sls_alignment_evaluer.hpp"

class EvalueComputation {
public:
    EvalueComputation(size_t dbResCount, BaseMatrix * subMat,
                      int gapOpen, int gapExtend, bool isGapped )
            : dbResCount(dbResCount)
    {
        const double lambdaTolerance = 0.01;
        const double kTolerance = 0.05;
        const double maxMegabytes = 500;
        const long randomSeed = 42; // we all know why 42
        const double maxSeconds = 60.0;
        Sls::AlignmentEvaluerParameters *par = NULL;

        const static EvalueParameters defaultParameter[] = {
                {"blosum62.out", 11, 1, true,  {0.27359865037097330642, 0.044620920658722244834,
                                                1.5938724404943873658, -19.959867650284412122,
                                                1.5938724404943873658, -19.959867650284412122,
                                                30.455610143099914211, -622.28684628915891608,
                                                30.455610143099914211, -622.28684628915891608,
                                                29.602444874818868215, -601.81087985041381216}},
                {"blosum62.out", 0,  0, false, {0.3207378152604042354,  0.13904657125294345166,
                                                0.76221128839920349041, 0,
                                                0.76221128839920349041, 0,
                                                4.5269915477182944841,  0,
                                                4.5269915477182944841,  0,
                                                4.5269915477182944841,  0}}
        };



        for (size_t i = 0; i < ARRAY_SIZE(defaultParameter); i++) {
            if(defaultParameter[i].matrixName == subMat->getMatrixName()){
                if ((fabs(defaultParameter[i].gapOpen - ((double) gapOpen)) < 0.1) &&
                    (fabs(defaultParameter[i].gapExtend - ((double) gapExtend)) < 0.1)&&
                    defaultParameter[i].isGapped == isGapped) {
                    par = (Sls::AlignmentEvaluerParameters*) &(defaultParameter[i].par);
                    break;
                }
            }
        }

        if(par!=NULL){
            evaluer.initParameters(*par);
        }else{
            long ** tmpMat = new long *[subMat->alphabetSize];
            long * tmpMatData = new long[subMat->alphabetSize*subMat->alphabetSize];
            for(int i = 0; i < subMat->alphabetSize; i++) {
                tmpMat[i] = &tmpMatData[i * subMat->alphabetSize];
                for (int j = 0; j < subMat->alphabetSize; j++) {
                    tmpMat[i][j] = subMat->subMatrix2Bit[i][j];
                }
            }
            if(isGapped) {
                //-1 to avoid X
                evaluer.initGapped(
                        subMat->alphabetSize-1, (const long *const *)tmpMat,
                        subMat->pBack, subMat->pBack,
                        gapOpen, gapExtend, gapOpen, gapExtend,
                        false, lambdaTolerance, kTolerance,
                        maxSeconds, maxMegabytes, randomSeed);
            }else{
                //subMat->alphabetSize-1
                evaluer.initGapless(
                        subMat->alphabetSize-1, (const long *const *)tmpMat,
                        subMat->pBack, subMat->pBack,
                        maxSeconds);
            }
            delete [] tmpMatData;
            delete [] tmpMat;
        }
        if(evaluer.isGood()==false){
            Debug(Debug::ERROR) << "ALP did not converge for the substitution matrix, gap open, gap extend input.\n"
                                   "Please change your input parameters. \n";
            EXIT(EXIT_FAILURE);
        }
        logK = log(evaluer.parameters().K);
    }

    ~EvalueComputation(){
    }

    inline double computeBitScore(double score) {
        return evaluer.bitScore(score, logK);
    }

    inline double computeRawScoreFromBitScore(double bitScore) {
        return (logK + bitScore * std::log(2.0)) / evaluer.parameters().lambda;
    }

    int minScore(double evalue, size_t qL) {
        // do log of evalue separately, to reduce the risk of overflow:
        double s = (std::log(evaluer.parameters().K * area(60, qL)) - std::log(evalue)) / evaluer.parameters().lambda;
        return std::ceil(std::max(1.0, s));
    }

    double area(double score, double seqLength){
        return evaluer.area( score, seqLength, dbResCount );
    }

    inline double computeEvalue(double score, double seqLength) {
        const double epa = evaluer.evaluePerArea( score );
        const double a = area( score, seqLength );
        return epa * a;
    }

private:
    Sls::AlignmentEvaluer evaluer;
    const size_t dbResCount;
    double logK;

    struct EvalueParameters {
        const std::string matrixName;
        int gapOpen;
        int gapExtend;
        bool isGapped;
        Sls::AlignmentEvaluerParameters par;
    };

};


#endif //MMSEQS_EVALUE_COMPUTATION_H

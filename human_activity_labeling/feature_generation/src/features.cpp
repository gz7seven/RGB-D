/**
1~40: current frame
41~436: against old frame (up to -65)
437~448: hand
449~454: foot
455~458: hand low high percentile
459: head angle
**/
#include<algorithm>

bool DEBUG_numFeature = false; // turn on to print out how many features are being recorded

int percentile[] = {10, 90}; //{10, 50, 90};    
bool HEAD_AGAINST_MID_HEAP = true;

bool FOOT_AGAINST_HEAD = false; // keep it false; FOOT against Torso is being done by default
bool USE_HAND_MOTION = false; // default: false.. not sure why it doesn't help..
bool USE_BIGGEST_UPDOWN_HAND_MOVE = false; // default: false



class FeaturesSkel {
private:
    double ***prevFrame; //[frameStoreNum][JOINT_NUM][JOINT_DATA_NUM]
    double ***prevFrame_posOnly;//[frameStoreNum][POS_JOINT_NUM][POS_JOINT_DATA_NUM];

    int currentFramePointer;
    bool frameComparable;
    bool mirrored;
    std::vector<double> featureValues;
    
    char* curFileName;
    //FILE * pRecFile;
    bool ready;
    
    void init(bool mirrored) {
    //void init(char* curFileNum, FILE* pRecFile_temp, bool mirrored) {
        //curFileName = curFileNum;
       // pRecFile = pRecFile_temp;
        currentFramePointer = -1;
        frameComparable = false;
        ready = true;
        this->mirrored = mirrored;

        //init prevFrame
        prevFrame = new double**[frameStoreNum]; 
        for (int i=0;i<frameStoreNum;i++) {
            prevFrame[i] = new double*[JOINT_NUM];
            for (int j=0;j<JOINT_NUM;j++) {
                prevFrame[i][j] = new double[JOINT_DATA_NUM];
            }
        }

        // init prevFrame_posOnly
        prevFrame_posOnly = new double**[frameStoreNum];
        for (int i=0;i<frameStoreNum;i++) {
            prevFrame_posOnly[i] = new double*[POS_JOINT_NUM];
            for (int j=0;j<POS_JOINT_NUM;j++) {
                prevFrame_posOnly[i][j] = new double[POS_JOINT_DATA_NUM];
            }
        }
        
    } // end init
    
    int findFrame(int diff) {
        //return ((currentFramePointer + diff) % frameStoreNum);
        int a = (currentFramePointer + diff);
        if (a >= frameStoreNum)
            return a - frameStoreNum;
        else if (a < 0)
            return a + frameStoreNum;
        else
            return a;
    }

    double* computeLocalHandLoc(double head_ori[9], double head_pos[3], double hand_pos[3]) {
        double handx = hand_pos[0] - head_pos[0];
        double handy = hand_pos[1] - head_pos[1];
        double handz = hand_pos[2] - head_pos[2];
        
        double* rel_hand = new double[3];
        
        rel_hand[0] = (head_ori[0]*handx + head_ori[3]*handy + head_ori[6]*handz) / 1000;
        rel_hand[1] = (head_ori[1]*handx + head_ori[4]*handy + head_ori[7]*handz) / 1000;
        rel_hand[2] = (head_ori[2]*handx + head_ori[5]*handy + head_ori[8]*handz) / 1000;
        
        //printf("%.2f \t%.2f \t%.2f\n", rel_hand[0], rel_hand[1], rel_hand[2]);
        
        return rel_hand;
    }

    int computeFootPosition(double **data, double **pos_data, int variableNum) {
        //double data[JOINT_NUM][JOINT_DATA_NUM], double pos_data[POS_JOINT_NUM][POS_JOINT_DATA_NUM], int variableNum) {
        
        // compute foot loc
        double left_foot_pos[3];
        double right_foot_pos[3];
        double head_ori[9];
        double head_pos[3];
        double torso_ori[9];
        double torso_pos[3];

        for (int i = 0;i<9;i++) {
            head_ori[i]=data[HEAD_JOINT_NUM][i];
        }
        for (int i=0;i<3;i++) {
            head_pos[i]=data[HEAD_JOINT_NUM][i+9];
        }
        for (int i = 0;i<9;i++) {
            torso_ori[i]=data[TORSO_JOINT_NUM][i];
        }
        for (int i=0;i<3;i++) {
            torso_pos[i]=data[TORSO_JOINT_NUM][i+9];
        }
        for (int i=0;i<3;i++) {
            left_foot_pos[i]=pos_data[POS_LEFT_FOOT_NUM][i];
        }
        for (int i=0;i<3;i++) {
            right_foot_pos[i]=pos_data[POS_RIGHT_FOOT_NUM][i];
        }
        
        double* left_foot;
        if (FOOT_AGAINST_HEAD) {
            left_foot = computeLocalHandLoc(head_ori, head_pos, left_foot_pos);
            for (int i=0;i<3;i++) {
           //     fprintf(pRecFile, "%.7f,",left_foot[i]);
                featureValues.push_back(left_foot[i]);
                variableNum++;
            }
        }
        left_foot = computeLocalHandLoc(torso_ori, torso_pos, left_foot_pos);
        for (int i=0;i<3;i++) {
          //  fprintf(pRecFile, "%.7f,", left_foot[i]);
            featureValues.push_back(left_foot[i]);
            variableNum++;
        } 
        
        
        double* right_foot;
        if (FOOT_AGAINST_HEAD) {
            right_foot = computeLocalHandLoc(head_ori, head_pos, right_foot_pos);
            for (int i=0;i<3;i++) {
          //      fprintf(pRecFile, "%.7f,", right_foot[i]);
                featureValues.push_back(right_foot[i]);
                variableNum++;
            }
        }
        right_foot = computeLocalHandLoc(torso_ori, torso_pos, right_foot_pos);
        for (int i=0;i<3;i++) {
         //   fprintf(pRecFile, "%.7f,", right_foot[i]);
            featureValues.push_back(right_foot[i]);
            variableNum++;
        }

        if (DEBUG_numFeature) printf("foot: %d\n", variableNum);
        
        return variableNum;
    } // end computeFootPosition    


    int computeHandPosition(double **data, double **pos_data, int variableNum) {
        //double data[JOINT_NUM][JOINT_DATA_NUM], double pos_data[POS_JOINT_NUM][POS_JOINT_DATA_NUM], int variableNum) {
        
        // compute hand loc
        double left_hand_pos[3];
        double right_hand_pos[3];
        double head_ori[9];
        double head_pos[3];
        double torso_ori[9];
        double torso_pos[3];

        for (int i = 0;i<9;i++) {
            head_ori[i]=data[HEAD_JOINT_NUM][i];
        }
        for (int i=0;i<3;i++) {
            head_pos[i]=data[HEAD_JOINT_NUM][i+9];
        }
        for (int i = 0;i<9;i++) {
            torso_ori[i]=data[TORSO_JOINT_NUM][i];
        }
        for (int i=0;i<3;i++) {
            torso_pos[i]=data[TORSO_JOINT_NUM][i+9];
        }
        for (int i=0;i<3;i++) {
            left_hand_pos[i]=pos_data[POS_LEFT_HAND_NUM][i];
        }
        for (int i=0;i<3;i++) {
            right_hand_pos[i]=pos_data[POS_RIGHT_HAND_NUM][i];
        }
        
        
        double* left_hand = computeLocalHandLoc(head_ori, head_pos, left_hand_pos);
        for (int i=0;i<3;i++) {
           // fprintf(pRecFile, "%.7f,",left_hand[i]);
            featureValues.push_back(left_hand[i]);
            variableNum++;
        }
        left_hand = computeLocalHandLoc(torso_ori, torso_pos, left_hand_pos);
        for (int i=0;i<3;i++) {
          //  fprintf(pRecFile, "%.7f,", left_hand[i]);
            featureValues.push_back(left_hand[i]);
            variableNum++;
        } 

        double* right_hand = computeLocalHandLoc(head_ori, head_pos, right_hand_pos);
        for (int i=0;i<3;i++) {
         //   fprintf(pRecFile, "%.7f,", right_hand[i]);
            featureValues.push_back(right_hand[i]);
            variableNum++;
        }
        right_hand = computeLocalHandLoc(torso_ori, torso_pos, right_hand_pos);
        for (int i=0;i<3;i++) {
          //  fprintf(pRecFile, "%.7f,", right_hand[i]);
            featureValues.push_back(right_hand[i]);
            variableNum++;
        }
        if (DEBUG_numFeature) printf("hand: %d\n", variableNum);
           
        // Maybe try collecting hand motion.
        if (USE_HAND_MOTION) {
            double left_hand_pos_prev[3];
            double right_hand_pos_prev[3];
            double diff_left[3];
            double diff_right[3];
            for (int i=0; i<compareFrameNum; i++) {
                int frameCompareTo = findFrame(compareFrame[i]);
                for (int j=0;j<3;j++) {
                    left_hand_pos_prev[j] = prevFrame_posOnly[frameCompareTo][POS_LEFT_HAND_NUM][j];
                    right_hand_pos_prev[j] = prevFrame_posOnly[frameCompareTo][POS_RIGHT_HAND_NUM][j];

                    diff_left[j] = left_hand_pos[j] - left_hand_pos_prev[j];
                    diff_right[j] = right_hand_pos[j] - right_hand_pos_prev[j];
                }
              //  fprintf(pRecFile, "%.7f,%.7f,%.7f,", diff_left[0], diff_left[1], diff_left[2]);
              //  fprintf(pRecFile, "%.7f,%.7f,%.7f,", diff_right[0], diff_right[1], diff_right[2]);
                for(int num=0; num<3; num++)
                {
                    featureValues.push_back(diff_left[num]);
                }
                for(int num=0; num<3; num++)
                {
                    featureValues.push_back(diff_right[num]);
                }
                variableNum+=6;
            }
            if (DEBUG_numFeature) printf("hand motion: %d\n", variableNum);
        }

        return variableNum;
    } // end computeHandPosition


    int computeMoreHandFeatures(int variableNum) {
        //double data[JOINT_NUM][JOINT_DATA_NUM], double pos_data[POS_JOINT_NUM][POS_JOINT_DATA_NUM], int variableNum) {
        
        // compute hand loc
        double left_hand_pos[3];
        double right_hand_pos[3];
        double head_ori[9];
        double head_pos[3];

        int latest = compareFrame[compareFrameNum-1];
        vector<double> lhand(30);
        vector<double> rhand(30);
        int vCount=0;
        for (int last=-2;last>latest;last--) {
            int frameN = findFrame(last);

            double ltemp = prevFrame_posOnly[frameN][POS_LEFT_HAND_NUM][1]-prevFrame[frameN][HEAD_JOINT_NUM][9+1]; // y value
            double rtemp = prevFrame_posOnly[frameN][POS_RIGHT_HAND_NUM][1]-prevFrame[frameN][HEAD_JOINT_NUM][9+1]; // y value
            lhand.push_back(ltemp);
            rhand.push_back(rtemp);
            vCount++;
            //cout << "l " << ltemp << " " << rtemp << endl;
            /*for (int i = 0;i<9;i++) {
                head_ori[i]=prevFrame[frameN][HEAD_JOINT_NUM][i];
            }
            for (int i=0;i<3;i++) {
                head_pos[i]=prevFrame[frameN][HEAD_JOINT_NUM][i+9];
            }
            for (int i=0;i<3;i++) {
                left_hand_pos[i]=prevFrame_posOnly[frameN][POS_LEFT_HAND_NUM][i];
            }
            for (int i=0;i<3;i++) {
                right_hand_pos[i]=prevFrame_posOnly[frameN][POS_RIGHT_HAND_NUM][i];
            }
            
            double* left_hand = computeLocalHandLoc(head_ori, head_pos, left_hand_pos);
            lhand.push_back(left_hand[2]);

            double* right_hand = computeLocalHandLoc(head_ori, head_pos, right_hand_pos);
            rhand.push_back(right_hand[2]);

            for (int j=0;j<3;j++) {
                cout << right_hand[j] << " ";
            }
            cout << endl;*/
        }
        
        sort(lhand.begin(), lhand.end());
        sort(rhand.begin(), rhand.end());

        int numValue = vCount;

        for (int i=0;i<sizeof(percentile)/sizeof(percentile[0]);i++) {
            int temp = numValue * percentile[i] / 100;
           // fprintf(pRecFile, "%.1f,%.1f,", lhand[temp], rhand[temp]);
            
            featureValues.push_back(lhand[temp]);
            featureValues.push_back(rhand[temp]);
            
            variableNum += 2;
            //cout << i << " " << temp << " "  << numValue << " " << latest <<  " " << lhand[temp] << " " << rhand[temp] << endl;
            if (DEBUG_numFeature) printf("hand location (percentile: %d): %d\n", percentile[i], variableNum);            
        }

        if (USE_BIGGEST_UPDOWN_HAND_MOVE) {
            int temp1 = numValue * 5 / 100;
            int temp2 = numValue * 95 / 100;
            double moveL = lhand[temp2]-lhand[temp1];
            double moveR = rhand[temp2]-rhand[temp1];
           // fprintf(pRecFile, "%.1f,%.1f,", moveL, moveR);
            featureValues.push_back(moveL);
            featureValues.push_back(moveR);
            variableNum += 2;
            if (DEBUG_numFeature) printf("hand biggest move (up/down): %d\n", variableNum);            
        }
        
        if (DEBUG_numFeature) printf("extra hand features (percentile+move): %d\n", variableNum);
        return variableNum;
    } // end computeMoreHandFeatures

    int computeHead(int variableNum) {
        //double data[JOINT_NUM][JOINT_DATA_NUM], double pos_data[POS_JOINT_NUM][POS_JOINT_DATA_NUM], int variableNum) {

        int curFrame = findFrame(0);
        
        // compute hand loc
        double heap_pos[3];
        double head_pos[3];
        double head_up[3];

        double he_fs = 0; // head from sensor
        double hp_fs = 0; // heap from sensor

        //double head_ori[9];
        /*for (int i=0;i<9;i++) {
            head_ori[i] = prevFrame[curFrame][HEAD_JOINT_NUM][i];
        }*/
        for (int i=0;i<3;i++) {
            double ltemp = prevFrame[curFrame][7][i+9];
            double rtemp = prevFrame[curFrame][9][i+9];
            heap_pos[i] = (ltemp+rtemp)/2;

            head_pos[i] = prevFrame[curFrame][HEAD_JOINT_NUM][i+9];

            head_up[i] = heap_pos[i];
            if (i==1) { // y val
                head_up[i] = head_pos[i];
            }
            
            if (i!=1) { // except y val
                he_fs += (head_pos[i]*head_pos[i]);
                hp_fs += (heap_pos[i]*heap_pos[i]);
            }
            //printf("%d: head: %.1f heap: %.1f up: %.1f\n", i+1, head_pos[i], heap_pos[i], head_up[i]);
        }
        
        double v1[3]; // heap to head
        double v2[3]; // heap to up
        double v1_det = 0;
        double v2_det = 0;
        double dot = 0;

        for (int i=0;i<3;i++) {
            v1[i] = head_pos[i] - heap_pos[i];
            v2[i] = head_up[i] - heap_pos[i];

            dot += (v1[i]*v2[i]);
            v1_det += (v1[i]*v1[i]);
            v2_det += (v2[i]*v2[i]);
        }
        v1_det = sqrt(v1_det);
        v2_det = sqrt(v2_det);

        double ang = acos(dot / (v1_det * v2_det));

        ang = ang * 180 / 3.14159265; // to degree
        
        if (he_fs > hp_fs) { // if heap is closer
            ang = -ang;
        }
        
        if (ang>80) {
        //    printf("computeHead angle not right.. %.1f \n", ang); exit(1);
        }
        
      //  fprintf(pRecFile, "%.1f,", ang);
        featureValues.push_back(ang);
        variableNum++;
        

        /*double* headLoc = computeLocalHandLoc(head_ori, head_pos, heap_pos);
        for (int i=0;i<3;i++) {
            fprintf(pRecFile, "%.7f,", headLoc[i]);
            variableNum++;
        }*/
        
        if (DEBUG_numFeature) printf("HEAD angle feature: %d\n", variableNum);
        return variableNum;
    } // end computeHead
    
    int computeBodyPoseAndMotionInfo(double **data, double **pos_data, int variableNum) { 
        //double data[JOINT_NUM][JOINT_DATA_NUM], double pos_data[POS_JOINT_NUM][POS_JOINT_DATA_NUM], int variableNum) {
        for (int i=0; i<compareFrameNum; i++) {
            for (int j=0; j<JOINT_NUM; j++) {

                int frameCompareTo = findFrame(compareFrame[i]);
                
                if (currentFramePointer == frameCompareTo) {
                    if (j != TORSO_JOINT_NUM) {
                        // rot mat
                        double m[3][3];
                       
                        for (int k =0;k<3;k++) {
                            m[0][k]=  prevFrame[currentFramePointer][TORSO_JOINT_NUM][0]*prevFrame[currentFramePointer][j][0+k]
                                    + prevFrame[currentFramePointer][TORSO_JOINT_NUM][3]*prevFrame[currentFramePointer][j][3+k]
                                    + prevFrame[currentFramePointer][TORSO_JOINT_NUM][6]*prevFrame[currentFramePointer][j][6+k];
                            m[1][k]=  prevFrame[currentFramePointer][TORSO_JOINT_NUM][1]*prevFrame[currentFramePointer][j][0+k]
                                    + prevFrame[currentFramePointer][TORSO_JOINT_NUM][4]*prevFrame[currentFramePointer][j][3+k]
                                    + prevFrame[currentFramePointer][TORSO_JOINT_NUM][7]*prevFrame[currentFramePointer][j][6+k];
                            m[2][k]=  prevFrame[currentFramePointer][TORSO_JOINT_NUM][2]*prevFrame[currentFramePointer][j][0+k]
                                    + prevFrame[currentFramePointer][TORSO_JOINT_NUM][5]*prevFrame[currentFramePointer][j][3+k]
                                    + prevFrame[currentFramePointer][TORSO_JOINT_NUM][8]*prevFrame[currentFramePointer][j][6+k];
                                    
                            /*printf("%.10f %.10f %.10f %.10f\n",  prevFrame[currentFramePointer][TORSO_JOINT_NUM][0]*prevFrame[currentFramePointer][j][0+k]
                                    , prevFrame[currentFramePointer][TORSO_JOINT_NUM][3]*prevFrame[currentFramePointer][j][3+k]
                                    , prevFrame[currentFramePointer][TORSO_JOINT_NUM][6]*prevFrame[currentFramePointer][j][6+k], m[0][k]);*/
                        }
                        /*for (int ii = 0;ii<3;ii++) 
                            for(int jj=0;jj<3;jj++) 
                                printf("%d %d %d %.10f\n",frameCompareTo, ii,jj,m[ii][jj]); */
                        // quat
                        double q1 = 1.0/2.0 * sqrt(1 + m[0][0] - m[1][1] - m[2][2]);
                        double q2 = 1.0/(4.0*q1) * (m[0][1] + m[1][0]);
                        double q3 = 1.0/(4.0*q1) * (m[0][2] + m[2][0]);
                        double q4 = 1.0/(4.0*q1) * (m[2][1] - m[1][2]);
                        
                        if (!isnan(q1) && !isnan(q2) && !isnan(q3) && !isnan(q4) 
                                && !isinf(q1) && !isinf(q2) && !isinf(q3) && !isinf(q4)) {
                        //    fprintf(pRecFile, "%.7f,%.7f,%.7f,%.7f,",q1,q2,q3,q4);
                            featureValues.push_back(q1);
                            featureValues.push_back(q2);
                            featureValues.push_back(q3);
                            featureValues.push_back(q4);
                        } else {
                            double zero = 0.0;
                      //      fprintf(pRecFile, "%.1f,%.1f,%.1f,%.1f,",zero,zero,zero,zero);;
                            for(int num =0; num<4; num++)
                                featureValues.push_back(zero);
                        }
                        variableNum+=4;
                        if(DEBUG_numFeature) printf("current frame : %d\n", variableNum);
                    }
                } else {
                    // rot mat
                    double m[3][3];
                    for (int k =0;k<3;k++) {
                        m[0][k]=  prevFrame[frameCompareTo][j][0]*prevFrame[currentFramePointer][j][0+k]
                                + prevFrame[frameCompareTo][j][3]*prevFrame[currentFramePointer][j][3+k]
                                + prevFrame[frameCompareTo][j][6]*prevFrame[currentFramePointer][j][6+k];
                        m[1][k]=  prevFrame[frameCompareTo][j][1]*prevFrame[currentFramePointer][j][0+k]
                                + prevFrame[frameCompareTo][j][4]*prevFrame[currentFramePointer][j][3+k]
                                + prevFrame[frameCompareTo][j][7]*prevFrame[currentFramePointer][j][6+k];
                        m[2][k]=  prevFrame[frameCompareTo][j][2]*prevFrame[currentFramePointer][j][0+k]
                                + prevFrame[frameCompareTo][j][5]*prevFrame[currentFramePointer][j][3+k]
                                + prevFrame[frameCompareTo][j][8]*prevFrame[currentFramePointer][j][6+k];
                    }
                    
                   /*for (int ii = 0;ii<3;ii++) 
                        for(int jj=0;jj<3;jj++) 
                            printf("%d %d %d %.10f\n",frameCompareTo, ii,jj,m[ii][jj]); */
                    
                    // quat
                    double q1 = 1.0/2.0 * sqrt(1 + m[0][0] - m[1][1] - m[2][2]);
                    double q2 = 1.0/(4.0*q1) * (m[0][1] + m[1][0]);
                    double q3 = 1.0/(4.0*q1) * (m[0][2] + m[2][0]);
                    double q4 = 1.0/(4.0*q1) * (m[2][1] - m[1][2]);
                    
                        if (!isnan(q1) && !isnan(q2) && !isnan(q3) && !isnan(q4) 
                                &&!isinf(q1) && !isinf(q2) && !isinf(q3) && !isinf(q4)) {
                       // fprintf(pRecFile, "%.7f,%.7f,%.7f,%.7f,",q1,q2,q3,q4);
                        featureValues.push_back(q1);
                        featureValues.push_back(q2);
                        featureValues.push_back(q3);
                        featureValues.push_back(q4);
                    } else {
                        double zero = 0.0;
                      //  fprintf(pRecFile, "%.1f,%.1f,%.1f,%.1f,",zero,zero,zero,zero);;
                        for(int num = 0; num < 4; num++)
                                featureValues.push_back(zero);
                    }
                            
                            
                    variableNum+=4;
                    if(DEBUG_numFeature) printf("comparing against frame : %d (against:%d)\n", variableNum, compareFrame[i]);   
                }
                
            } // end j
        } // end i
        
        return variableNum;
    } // end computeBodyPoseAndMotionInfo


   
    bool processData(double **data, double **pos_data) { 
        //double data[JOINT_NUM][JOINT_DATA_NUM], double pos_data[POS_JOINT_NUM][POS_JOINT_DATA_NUM]) {
        featureValues.clear();
        // advance one frame
        currentFramePointer=findFrame(1);
        
        // store current data
        for (int i=0;i<JOINT_NUM;i++) {
            for (int j=0;j<JOINT_DATA_NUM;j++) {
                prevFrame[currentFramePointer][i][j] = data[i][j];
            }
        }

        for (int i=0;i<POS_JOINT_NUM;i++) {        
            for (int j=0;j<POS_JOINT_DATA_NUM;j++) {
                prevFrame_posOnly[currentFramePointer][i][j] = pos_data[i][j];
            }
        }
        
        // compute diff
        if (currentFramePointer >= (frameStoreNum-1)) {
            frameComparable = true;
        }
        if (!frameComparable) {
            cout << "current frame pointer " << currentFramePointer << endl;
            if (currentFramePointer >= (frameStoreNum-1)) {
                frameComparable = true;
            }
        } else {
            int variableNum = 0;

            // each row starts with fileName            
            // add L or M at the beginning to tell it is label (helps reading from matlab)
           /* if (!mirrored) {
                fprintf(pRecFile, "L%s,", curFileName);
            } else {
                fprintf(pRecFile, "M%s,", curFileName);
            }*/
            //fprintf(pRecFile, "%d,", currentFramePointer);
            
            // feature 1 & 3 of AAAI paper
            variableNum = computeBodyPoseAndMotionInfo(data,pos_data,variableNum);
            
            // feature 2 of AAAI paper + some extra Hand motion (hand motion fails)
            variableNum = computeHandPosition(data,pos_data,variableNum);

            // foot location
            variableNum = computeFootPosition(data,pos_data,variableNum);

            // hand percentile
            variableNum = computeMoreHandFeatures(variableNum);

            if (HEAD_AGAINST_MID_HEAP) {
                variableNum = computeHead(variableNum);
            }

            if(DEBUG_numFeature) {
                printf("\t\tfeatures: %d\n", variableNum);
            }
            return true;            
        }
        return false;
    }

public:
    std::vector<double> getFeatureValues(){
        return featureValues;
    }
        bool addData(double **data, double **pos_data) { 
        //double data[JOINT_NUM][JOINT_DATA_NUM], double pos_data[POS_JOINT_NUM][POS_JOINT_DATA_NUM]) {
        
        // advance one frame
        currentFramePointer=findFrame(1);
        
        // store current data
        for (int i=0;i<JOINT_NUM;i++) {
            for (int j=0;j<JOINT_DATA_NUM;j++) {
                prevFrame[currentFramePointer][i][j] = data[i][j];
            }
        }

        for (int i=0;i<POS_JOINT_NUM;i++) {        
            for (int j=0;j<POS_JOINT_DATA_NUM;j++) {
                prevFrame_posOnly[currentFramePointer][i][j] = pos_data[i][j];
            }
        }
        if (currentFramePointer >= (frameStoreNum-1)) {
            frameComparable = true;
        }
    }
    
    // return true if feature extraction started and successful..
    bool extractSkeletonFeature(double **data, double **pos_data) { 
                            //double data[JOINT_NUM][JOINT_DATA_NUM],
                              //   double pos_data[POS_JOINT_NUM][POS_JOINT_DATA_NUM]) {
        if (ready) {
            return processData(data, pos_data);
    
        } else {
            printf("ERROR! feature extraction not ready.\n");            
            exit(1);
        }
        return false;
    }
    bool reset(bool mirrored){
        delete prevFrame;
        init(mirrored);
    }
    FeaturesSkel() {
        ready = false;
    }
    
   // FeaturesSkel(char* curFileNum, FILE* pRecFile_temp, bool mirrored) {
    //    init(curFileNum, pRecFile_temp, mirrored);
    //}
    FeaturesSkel(bool mirrored) {
        init(mirrored);
    }
    
    ~FeaturesSkel() {
        delete prevFrame;  
    }    

};


/** 
there are 11 joints that have both orientation (3x3) and position (x,y,z) data
		XN_SKEL_HEAD,
        XN_SKEL_NECK,
        XN_SKEL_TORSO,
        XN_SKEL_LEFT_SHOULDER,
        XN_SKEL_LEFT_ELBOW,
        XN_SKEL_RIGHT_SHOULDER,
        XN_SKEL_RIGHT_ELBOW,
        XN_SKEL_LEFT_HIP,
        XN_SKEL_LEFT_KNEE,
        XN_SKEL_RIGHT_HIP,
        XN_SKEL_RIGHT_KNEE
	
there are 4 joints that have only position (x,y,z) data
        XN_SKEL_LEFT_HAND,
        XN_SKEL_RIGHT_HAND,
        XN_SKEL_LEFT_FOOT,
        XN_SKEL_RIGHT_FOOT

 data[][0~8]    -> orientation (3x3 matrix)
                     3x3 matrix is stored as 
                        0 1 2
                        3 4 5
                        6 7 8
                     read PDF for description about 3x3 matrix 
 data[][9~11]   -> x,y,z position for eleven joints
 
 data_CONF[][0]   -> confidence value of orientation  (data[][0~8]) 
 data_CONF[][1]   -> confidence value of xyz position (data[][9~11])
 
 data_pos[][0~2] -> x,y,z position for four joints
 data_pos_CONF[]  -> confidence value of xyz position (data_pos[][0~2])
 
 IMAGE[X_RES][Y_RES][0~2]   -> RGB values
 IMAGE[X_RES][Y_RES][3]     -> depth values


 */

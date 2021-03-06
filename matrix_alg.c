#include "matrix_alg_ptypes.h"
#include "matrix_basicfunc_ptypes.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>


/**Gaussian elimination
*A parameterkent atadott matrix-ot atmasolja egy dinamikusan foglalt matrix-ba(nem az eredetit modositja),
*es azon Gauss-eliminaciot vegez majd visszater vele. Az algoritmus futasat lehet modositani a flag parameterrel.
*Ezek ertekei lehetnek a matrix_dtype.h-ban megtalalho makrok. Ha SOLVE_LINEAR_SYSTEM akkor egy uj matrixot foglal a fuggveny atmasolja ide a megoldando egyenletrendszert
*es ezen vegzi a muveleteket, majd ezzel is ter vissza. Ha CALCULATE_DET es a matrix nem negyzetes akkor NULL-al ter vissza, egyebkent kiszamolja a determinanst gauss eliminacioval
*egy masik matrixban es a vegen az eredeti determinant adattagjat modositja.
*@param matrix Az eliminalando matrix
*@param flag is a choice between determinant-calculator and linear equation solver mode.
*@return A kiszamolt matrix
*/
Matrix* matricks_gaussian_elimination(Matrix* matrix, const int flag) {
    Matrix* matrixCopy = matricks_malloc(matrix->rows, matrix->columns);
    matricks_copy(matrixCopy, matrix);

    int maxCol, maxRow;
    double D, lambda;

    switch(flag) {
        case SOLVE_LINEAR_SYSTEM:
            //eggyel kevesebb oszlopig kell menni! (A|b) alak
            maxCol = matrix->columns - 1;
            maxRow = matrix->rows;
            break;
        case CALCULATE_DET:
            //ha nem negyzetes nincs ertelme det szamolni
            if(!matricks_is_quadratic((matrix))) {
                return NULL;
            }
            D = 1;
            maxCol = matrix->columns;
            maxRow = matrix->rows;
            break;
        default:
            maxCol = matrix->columns;
            maxRow = matrix->rows;
            break;
    }

    //3 fazisu Gauss-eliminacio, gyakorlatilag egy allapotgep
    enum State{LOOP, CHANGEROW, FINISHED};
    enum State state = LOOP;

    //i,j a "kulso" valtozok(eszerint haladunk vegig a matrixon a gauss eliminacio algoritmusa szerint)
    //rowVar, colVar a muveletekhez szuksegesek
    bool finished = false;
    int i = 0, j = 0;
    int rowVar,colVar;
    Matrix* returnValue;
    while(!finished) {
        switch(state) {
            //Elso fazis
            case LOOP:
                //ha szam == 0, ugrunk a masodik fazisra
                if( !FloatCmp(matrixCopy->numbers[i][j], 0, 1.e-9) ){
                    state = CHANGEROW;
                    break;
                }
                //egyebkent beszorozzuk a sort az aktualis elem reciprokaval, es harmadik fazisba lepunk ha ez volt az utolso sor
                else{
                    //determinans szamolasanal amikor egy sort leosztunk akkor a determinans ertek is megvaltozik
                    //ezt kovetjuk vegig a D valtozoval
                    if(flag == CALCULATE_DET){
                        D *= matrixCopy->numbers[i][j];
                    }
                    lambda = 1 / matrixCopy->numbers[i][j];
                    matricks_multiply_row(matrixCopy, i, lambda);
                    if(i == matrixCopy->rows - 1){
                        state = FINISHED;
                        break;
                    }
                }
                //kinullazzuk az alatta levo sorokat
                for(rowVar = i + 1; rowVar < matrixCopy->rows; rowVar++){
                    //ha az oszlop alatt 0 all, nem is kell kinullazni
                    if( FloatCmp(matrixCopy->numbers[rowVar][j], 0, 1.e-9) ){
                        lambda = -matrixCopy->numbers[rowVar][j];
                        matricks_add_row(matrixCopy, i, rowVar, lambda);
                    }
                }
                //ha mar nincs tobb oszlop, 3. fazisra ugras
                if(j == maxCol - 1){
                    state = FINISHED;
                    break;
                }
                i++,j++;
                break;
            //2. fazis: sorcsere
            case CHANGEROW:
                //Ha van meg sor amelyre az adott oszlopban levo elem nem 0, akkor csere
                if(i < matrixCopy->rows - 1){
                    rowVar = i + 1;
                    bool notzero = false;
                    while(!notzero && rowVar < matrixCopy->rows){
                        if( FloatCmp(matrixCopy->numbers[rowVar][j], 0, 1.e-9) ){
                            notzero = true;
                        }
                        else rowVar++;
                    }
                    if(notzero){
                        if(flag == CALCULATE_DET){
                            D = -D;
                        }
                        matricks_change_row(matrixCopy,i,rowVar);
                        state = LOOP;
                        break;
                    }
                    else if(flag == CALCULATE_DET){
                        D = 0;
                        state = FINISHED;
                        break;
                    }
                }
                //elertuk az utolso oszlopot -> vegeztunk
                if(j == maxCol - 1){
                    i--;
                    state = FINISHED;
                    break;
                }
                //egyebkent folytatodjon a ciklus
                j++;
                state = LOOP;
                break;
            case FINISHED:
                //a maradek csupa nulla sorok torlese
                if(i < matrixCopy->rows - 1){
                    for(rowVar = i + 1; rowVar < matrixCopy->rows; rowVar++){
                        if(flag == SOLVE_LINEAR_SYSTEM){
                            if(FloatCmp(matrixCopy->numbers[rowVar][matrix->columns-1], 0, 1.e-9)){
                                break;
                            }
                        }
                        matricks_delete_row(matrixCopy,rowVar);
                    }
                }
                switch(flag){
                    case SOLVE_LINEAR_SYSTEM:
                        //Redukalt lepcsos alak
                        for(i = matrixCopy->rows - 1; i > 0; i--){
                           for(colVar = 0; colVar < maxCol; colVar++){
                                if(matrixCopy->numbers[i][colVar] != 0){
                                    j = colVar;
                                    break;
                                }
                            }
                            for(rowVar = i - 1 ; rowVar >= 0; rowVar--){
                                lambda = -matrixCopy->numbers[rowVar][j];
                                matricks_add_row(matrixCopy, i, rowVar, lambda);
                            }
                        }
                        returnValue = matrixCopy;
                        break;
                    case CALCULATE_DET:
                        matrix->determinant = D;
                        matricks_free(matrixCopy);
                        returnValue = matrix;
                        break;

                }
                finished = true;
                //printf("A matrix lepcsos alaku!");
                break;
        }
    }
    return returnValue;
}

int FloatCmp(double a, double b,double precision){
    if(fabs(a - b) < precision) return 0;
    else if(a > b)              return 1;
    else                        return -1;
}

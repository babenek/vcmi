/*******************************************************************
* addapted by Trevor Standley for use as a function approximator
* Addapted from:
* Basic Feed Forward Neural Network Class
* ------------------------------------------------------------------
* Bobby Anguelov - takinginitiative.wordpress.com (2008)
* MSN & email: banguelov@cs.up.ac.za
********************************************************************/
#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H
//standard includes
#pragma warning(push, 0)
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <limits>
#include "../../global.h"
#pragma warning(pop)

class neuralNetwork
{
public:
  //constructor & destructor
  neuralNetwork(si16 numInput, si16 numHidden1, si16 numHidden2, si16 numOutput);
  neuralNetwork(const neuralNetwork&);
  neuralNetwork();
  void operator = (const neuralNetwork&);
  ~neuralNetwork();

  //weight operations
  double* feedForwardPattern( double* pattern );
  void backpropigate(double* pattern, double OLR, double H2LR, double H1LR );

  void tweakWeights(double howMuch);
  void mate(const neuralNetwork&n1,const neuralNetwork&n2);

private:
	//number of neurons
	si16 nInput;
	si16 nHidden1;
	si16 nHidden2;
	si16 nOutput;
	
	//neurons
	double* inputNeurons;
	double* hiddenNeurons1;
	double* hiddenNeurons2;
	double* outputNeurons;

	//weights
	double** wInputHidden;
	double** wHidden2Hidden;
	double** wHiddenOutput;
	
	void initializeWeights();
	double activationFunction(double x);
	void feedForward(double* pattern);
	static float norm(void);
};

std::istream & operator >> (std::istream &, neuralNetwork & ann);
std::ostream & operator << (std::ostream &, const neuralNetwork & ann);
#endif
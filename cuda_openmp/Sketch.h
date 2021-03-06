#include <iostream>
#include <fstream>
#include <string.h>
#include <math.h>
#include <stdlib.h>

using namespace std;

// sketch abstraction
class Sketch {
	private:
		int numPoints,numStrokes;				// # points and strokes
		int ptAdded,strAdded;					// # of points and strokes added so far
		double **coords;						// array of point coordinates
		int *strokeIndices;						// array of starting indices of each stroke
		void getCentroid(double &x, double &y);	// a method to get the centroid
		void getStd(double &x, double &y);		// a method to get the std. in both x & y axes
		double findMaxDistance();				// a method to find the max. distance from the centroid
	public:
		Sketch(int,int);						// constructor
		~Sketch();								// destructor
		void addPoint(double,double);			// point adder method
		void openStroke();						// to open a stroke
		void printContents();					// to print the coordinates of points contained in each stroke
		int getNumPoints();						// returns # of points
		int getNumStrokes();					// returns # of strokes
		int *getStrokeIndices();				// returns the array containing starting points of every stroke
		double **getCoords();					// returns the array of point coordinates
		
		Sketch* resample(double rate);			// sketch resampler
		Sketch* normalized();					// normalization of a sketch
		Sketch* transform(double minX, double minY, double maxX, double maxY);
};


// constructor
Sketch::Sketch(int numPoints,int numStrokes) : numPoints(numPoints), numStrokes(numStrokes), ptAdded(0), strAdded(0) {
	//coords = new double*[numPoints];			// allocation of coordinate array
	//for ( int i = 0; i < numPoints; ++i) {
		//coords[i] = new double[2];
	//}
	
	coords = new double*[2];
	coords[0] = new double[numPoints];
	coords[1] = new double[numPoints];
	
	strokeIndices = new int[numStrokes];		// allocation of stroke indices array
}

// destructor
Sketch::~Sketch() {
	/*
	for ( int i = 0; i < numPoints; ++i) {		// deallocation of coordinate array
		delete [] coords[i];
	}*/
	
	delete [] coords[0];
	delete [] coords[1];
	delete [] coords;
	
	delete [] strokeIndices;					// deallocation of stroke indices array
}

// returns the centroid of the sketch
void Sketch::getCentroid(double &x, double &y) {
	if ( numPoints > 0) {						// if there are some points
		double xsum = 0;						// then take the avg. of coordinates
		double ysum = 0;						// and return them in parameters
		
		for (int i = 0; i < ptAdded; ++i) {
			xsum += coords[0][i];
			ysum += coords[1][i];
		}
		
		xsum /= numPoints;
		ysum /= numPoints;
		
		x = xsum;
		y = ysum;
	}
	else {
		x = y = 0;
	}
}

// returns the std. in both axes
void Sketch::getStd(double &x, double &y) {
	// if there are some points
	if ( numPoints > 0) {
		double cx,cy;
		getCentroid(cx,cy);										// take the std. in both axes
																// by just applying the defn. of
		double xsqsum = 0,ysqsum = 0;							// standard deviation
		for ( int i = 0; i < ptAdded; ++i) {
			xsqsum += (coords[0][i] - cx)*(coords[0][i] - cx);
			ysqsum += (coords[1][i] - cy)*(coords[1][i] - cy);
		}
		
		xsqsum /= numPoints;
		ysqsum /= numPoints;
		
		x = sqrt(xsqsum);
		y = sqrt(ysqsum);
	}
	else {
		x = y = 0;
	}
}

double Sketch::findMaxDistance() {
	// find the maximum distance from the centroid that can be possible,
	double x,y;
	getCentroid(x,y);
	
	double maxdist = -1;
	double curdist;
	for (int i = 0; i < ptAdded; ++i) {
		curdist = (coords[1][i] - y)*(coords[1][i] - y) + (coords[0][i] - x)*(coords[0][i] - x);
		
		if (curdist > maxdist) {
			maxdist = curdist;
		}
	}
	
	// return this distance (before returning that I took the sqrt of it here)
	return sqrt(maxdist);
}

// normalization of a sketch
Sketch* Sketch::normalized() {
	double cx,cy;
	double stdx,stdy;
	
	getCentroid(cx,cy);
	getStd(stdx,stdy);
	
	// store the normalized sketch in a new one
	Sketch *newSketch = new Sketch(numPoints,numStrokes);
	
	int upperBound;
	
	for ( int i = 0; i < strAdded; ++i) {
		newSketch->openStroke();
		
		if ( i == numStrokes - 1) {
			upperBound = ptAdded;
		}
		else {
			upperBound = strokeIndices[i+1];
		}
		
		// for each pt, translate the point to the origin
		// and normalize coordinates in both axes by their 
		// corresponding std.
		for ( int j = strokeIndices[i]; j < upperBound; ++j) {
			newSketch->addPoint((coords[0][j]-cx)/stdx,(coords[1][j]-cy)/stdy);
		}
	}
	
	// return the resulting sketch
	return newSketch;
}

// sketch resampler
Sketch* Sketch::resample(double rate) {
	int newNumPoints = 0;
	int upperBound,dist;
	
	// take the sampling interval
	double samplingInterval = findMaxDistance()*1.01 / rate;
	
	// we need to create a new sketch, however, we don't yet know
	// how many points will be created and added to this sketch
	// here I count the # of points needed
	for (int i = 0; i < strAdded; ++i) {
		++newNumPoints;
		
		if (i < numStrokes - 1) {
			upperBound = strokeIndices[i+1];
		}
		else {
			upperBound = ptAdded;
		}
		
		for (int j = strokeIndices[i]+1; j < upperBound; ++j) {
			// count the # of points between every point
			dist = sqrt((coords[0][j]-coords[0][j-1])*(coords[0][j]-coords[0][j-1]) + (coords[1][j]-coords[1][j-1])*(coords[1][j]-coords[1][j-1]));
			newNumPoints += (int) (ceil(dist / samplingInterval));
		}
	}
	
	// create the resampled sketch
	Sketch* resampled = new Sketch(newNumPoints,numStrokes);
	
	double prevx,prevy,sampdistance,cx,cy,angle,newx,newy;
	for (int i = 0; i < strAdded; ++i) {
		// before I start resampling, I need to open a stroke
		resampled->openStroke();
		
		prevx = coords[0][strokeIndices[i]];
		prevy = coords[1][strokeIndices[i]];
		
		resampled->addPoint(prevx,prevy);
		
		if (i < numStrokes - 1) {
			upperBound = strokeIndices[i+1];
		}
		else {
			upperBound = ptAdded;
		}
		
		// keep adding sample points far from a distance of sampling
		// interval, unless the lastly added point is closer than this
		// interval
		for (int j = strokeIndices[i]+1; j < upperBound; ++j) {
			sampdistance = sqrt((coords[0][j]-prevx)*(coords[0][j]-prevx) + (coords[1][j]-prevy)*(coords[1][j]-prevy));
			
			while (sampdistance > samplingInterval) {
				cx = prevx;
				cy = prevy;
				angle = atan2(coords[1][j]-cy, coords[0][j]-cx);
				newx = cos(angle)*samplingInterval + cx;
				newy = sin(angle)*samplingInterval + cy;
				prevx = newx;
				prevy = newy;
				
				// add the new sampled point
				resampled->addPoint(newx,newy);
				
				sampdistance = sqrt((coords[0][j]-prevx)*(coords[0][j]-prevx) + (coords[1][j]-prevy)*(coords[1][j]-prevy));
			}
		}
	}
	
	return resampled;
}

Sketch* Sketch::transform(double minX, double minY, double maxX, double maxY)
{
	double newMax = 23, newMin = 0;
	double newRange = newMax - newMin;
	double oldRangeX = maxX - minX;
	double oldRangeY = maxY - minY;
	Sketch *transformed= new Sketch(numPoints,numStrokes);
	transformed->ptAdded = this->ptAdded;
	transformed->strAdded = this->strAdded;
	for(int i = 0; i < strAdded; ++i)
	{
		transformed->strokeIndices[i] = strokeIndices[i];
	}

	for(int i = 0; i < ptAdded; ++i)
	{
		if(oldRangeX != 0)
		{
			transformed->coords[0][i] = ((coords[0][i] - minX)*newRange/oldRangeX) + newMin;
			transformed->coords[0][i] = floor(transformed->coords[0][i]);
		}
		if(oldRangeY != 0)
		{
			transformed->coords[1][i] = ((coords[1][i] - minY)*newRange/oldRangeY) + newMin;
			transformed->coords[1][i] = floor(transformed->coords[1][i]);
		}
	}

	return transformed;
}

// point adder
void Sketch::addPoint(double x, double y) {
	// if we haven't completely filled up our sketch
	if (ptAdded < numPoints) {
		// add the given point
		coords[0][ptAdded] = x;
		coords[1][ptAdded++] = y;
	}
	else {
		// allocate extra memory in case the memory is not sufficient
		coords[0] = (double *) realloc(coords[0],sizeof(double)*(2*numStrokes+numPoints));
		coords[1] = (double *) realloc(coords[1],sizeof(double)*(2*numStrokes+numPoints));
		coords[0][ptAdded] = x;
		coords[1][ptAdded] = y;
		++ptAdded; numPoints += 2*numStrokes;
	}
}

// to open a new stroke
void Sketch::openStroke() {
	// if there are un-opened strokes
	if (strAdded < numStrokes) {
		// add the starting index of the new stroke
		strokeIndices[strAdded++] = ptAdded;
	}
}

// print the contents of a sketch
void Sketch::printContents() {
	int upperBound;
	
	for ( int i = 0; i < strAdded; ++i) {
		cout << i << "=>";
		
		if ( i == numStrokes - 1) {
			upperBound = ptAdded;
		}
		else {
			upperBound = strokeIndices[i+1];
		}
		
		for ( int j = strokeIndices[i]; j < upperBound; ++j) {
			cout << "(" << coords[0][j] << "," << coords[1][j] << ")";
			
			if ( j < upperBound - 1) {
				cout << "-";
			} 
		}
		
		cout << endl;
	}
} 

// getter methods
int Sketch::getNumPoints() {
	return ptAdded;
}

int Sketch::getNumStrokes() {
	return strAdded;
}

double** Sketch::getCoords() {
	return coords;
}

int* Sketch::getStrokeIndices() {
	return strokeIndices;
}

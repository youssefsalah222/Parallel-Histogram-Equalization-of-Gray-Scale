#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#include <mpi.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

int main()
{
	MPI_Init(NULL, NULL);
	int count;
	MPI_Comm_size(MPI_COMM_WORLD, &count);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int ImageWidth = 4, ImageHeight = 4;
	int start_s, stop_s, TotalTime = 0;
	System::String^ imagePath;
	std::string img;
	//img = "C://Users//Abdo Ali//Desktop//image//inn.jpg";
	img = "..//Data//Input//N.png";
	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	int* out = inputImage(&ImageWidth, &ImageHeight, imagePath);
	int total = ImageWidth * ImageHeight;
	int histogram[256] = { 0 };
	int histogramm[256] = { 0 };
	float probility[256] = { 0.0 };
	float probilityy[64] = { 0.0 };
	float cumulativeprobility[256] = { 0.0 };
	int newhist[64] = { 0 };
	float fnewhist[64] = { 0.0 };
	int floorround[265] = { 0 };

	start_s = clock();
	//genrate histogram and calculate no of pixels of ins
	MPI_Scatter(imageData, total / 4, MPI_INT, out, total / 4, MPI_INT, 0, MPI_COMM_WORLD);
	for (int i = 0; i < total / 4; ++i) {
		histogram[out[i]]++;
	}
	MPI_Reduce(&histogram, &histogramm, 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Bcast(&histogramm, 256, MPI_FLOAT, 0, MPI_COMM_WORLD);
	//calculate the probility of each pixel
	MPI_Scatter(&histogramm, 64, MPI_INT, &newhist, 64, MPI_INT, 0, MPI_COMM_WORLD);
	for (int i = 0; i < 64; i++)
	{
		probilityy[i] = (double)newhist[i] / total;
	}
	MPI_Gather(&probilityy, 64, MPI_FLOAT, &probility, 64, MPI_FLOAT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&probility, 256, MPI_FLOAT, 0, MPI_COMM_WORLD);
	//calculate cumulative probility of each pixel
	cumulativeprobility[0] = probility[0];
	for (int i = 1; i < 256; i++)
	{
		cumulativeprobility[i] = probility[i] + cumulativeprobility[i - 1];
	}
	//scale the image and floor it
	MPI_Scatter(&cumulativeprobility, 64, MPI_FLOAT, &fnewhist, 64, MPI_FLOAT, 0, MPI_COMM_WORLD);
	for (int i = 0; i < 64; i++)
	{
		newhist[i] = floor((double)fnewhist[i] * 256);
	}
	MPI_Gather(&newhist, 64, MPI_INT, &floorround, 64, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&floorround, 256, MPI_INT, 0, MPI_COMM_WORLD);
	//here return every coler to his element
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < total; j++)
		{
			if (i == imageData[j])
			{
				out[j] = floorround[i];
			}
		}
	}
	stop_s = clock();
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	//display the image
	if (rank == 0)
	{
		cout << "time: " << TotalTime << endl;
		createImage(out, ImageWidth, ImageHeight, 0);
		free(imageData);
	}
	MPI_Finalize();
}
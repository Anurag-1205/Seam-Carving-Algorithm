#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

//g++ main.cpp `pkg-config --cflags --libs opencv4`

// ------ FUNCTION PROTOTYPES ------
unsigned int** create_matrix(unsigned int height, unsigned int width);
void free_matrix(unsigned int** arr, unsigned int height);
void cal_energy(Mat img, unsigned int** img_energy);
void cal_dp(unsigned int** img_energy, unsigned int** dp_matrix, unsigned int height, unsigned int width);
unsigned int* find_seam(unsigned int** dp_matrix, unsigned int height, unsigned int width);
Mat remove_seam(const Mat& img, unsigned int* seam);
void draw_seam(Mat& img, unsigned int* seam);

// ------ MAIN FUNCTION ------
int main(int argc, char* argv[]) {
    if(argc!=2){
        cout<<"Input format: ./a.out <image_path>";
        return 1;
    }
    string img_path = argv[1];
    Mat img = imread(img_path);
    if(img.empty()) {
        cout<<"Error: No image found.\n";
        return -1;
    }

    unsigned int height = img.rows;
    unsigned int width = img.cols;

    cout<<"Image Height: "<<height<<", Image Width: "<<width<<"\n";

    int tgt_width, tgt_height;
    cout<<"Enter desired Height: ";
    cin>>tgt_height;
    cout<<"Enter desired Width: ";
    cin>>tgt_width;

    if (tgt_width >= width || tgt_height > height) {
        cout<<"Target dimensions must be smaller than the original image dimensions to perform algorithm.\n";
        return 0;
    }

    string algo = "Seam Carving Animation";
    namedWindow(algo, WINDOW_AUTOSIZE);

    // --- VERTICAL SEAM REMOVAL ---
    int remove_col = width-tgt_width;
    for(int i=0; i<remove_col; i++) {
        unsigned int** energy_map = create_matrix(img.rows, img.cols);
        unsigned int** dp_map = create_matrix(img.rows, img.cols);

        cal_energy(img, energy_map);
        cal_dp(energy_map, dp_map, img.rows, img.cols);
        unsigned int* seam = find_seam(dp_map, img.rows, img.cols);
        
        Mat img_with_seam = img.clone();
        draw_seam(img_with_seam, seam);
        imshow(algo, img_with_seam);
        waitKey(1); 
        img = remove_seam(img, seam);

        free_matrix(energy_map, img.rows);
        free_matrix(dp_map, img.rows);
        delete[] seam;
    }
    
    // --- HORIZONTAL SEAM REMOVAL ---
    int remove_row = height-tgt_height;
    for(int i = 0; i < remove_row; i++) {
        Mat transposed_img;
        transpose(img, transposed_img);

        unsigned int current_height = transposed_img.rows;
        unsigned int current_width = transposed_img.cols;

        unsigned int** energy_map = create_matrix(current_height, current_width);
        unsigned int** dp_map = create_matrix(current_height, current_width);

        cal_energy(transposed_img, energy_map);
        cal_dp(energy_map, dp_map, current_height, current_width);
        unsigned int* seam = find_seam(dp_map, current_height, current_width);

        Mat trans_seam = transposed_img.clone();
        
        draw_seam(trans_seam, seam);
        
        Mat display_img;
        transpose(trans_seam, display_img);
        imshow(algo, display_img);
        waitKey(1);
        
        Mat f_img = remove_seam(transposed_img, seam);
        transpose(f_img, img); 

        free_matrix(energy_map, current_height);
        free_matrix(dp_map, current_height);
        delete[] seam;
    }

    cout << "Seam carving complete!\n";
    imshow("Resized Image", img);
    imwrite("output_resized.png", img);
    return 0;
}

// ------ FUNCTION IMPLEMENTATIONS ------

unsigned int** create_matrix(unsigned int height, unsigned int width){
    unsigned int** arr = new unsigned int*[height];
    for(unsigned int i=0; i<height; i++){
        arr[i] = new unsigned int[width];
    }
    return arr;
}

void free_matrix(unsigned int** arr, unsigned int height){
    for(unsigned int i=0; i<height; i++){
        delete[] arr[i];
    }
    delete[] arr;
}

void cal_energy(Mat img, unsigned int** img_energy){
    unsigned int height = img.rows; 
    unsigned int width = img.cols; 
    for(unsigned int i=0; i<height; i++){
        for(unsigned int j=0; j<width; j++){

            unsigned char b_left, g_left, r_left, b_right, g_right, r_right, b_up, g_up, r_up, b_down, g_down, r_down;

            unsigned int left_ind = (j==0)? (width-1) : (j-1);
            unsigned int right_ind = (j==width-1)? 0 : (j+1);
            unsigned int up_ind = (i==0)? (height-1) : (i-1);
            unsigned int down_ind = (i==height-1)? 0 : (i+1);
            
            
            Vec3b up_pixel = img.at<Vec3b>(up_ind, j);
            b_up = up_pixel[0];
            g_up = up_pixel[1];
            r_up = up_pixel[2];

            Vec3b left_pixel = img.at<Vec3b>(i, left_ind);
            b_left = left_pixel[0]; 
            g_left = left_pixel[1];
            r_left = left_pixel[2];
            
            Vec3b down_pixel = img.at<Vec3b>(down_ind, j);
            b_down = down_pixel[0];
            g_down = down_pixel[1];
            r_down = down_pixel[2];

            Vec3b right_pixel = img.at<Vec3b>(i, right_ind);
            b_right = right_pixel[0];
            g_right = right_pixel[1];
            r_right = right_pixel[2];

            //Calculating energy of pixel
            unsigned int value = pow((b_left - b_right), 2) + pow((g_left - g_right), 2) + pow((r_left - r_right), 2);
            value += pow((b_up - b_down), 2) + pow((g_up - g_down), 2) + pow((r_up - r_down), 2);
            img_energy[i][j] = pow(value, 0.5);
        }
    }
}

void cal_dp(unsigned int** img_energy, unsigned int** dp_matrix, unsigned int height, unsigned int width){
    for(unsigned int i=0; i<width; i++){
        dp_matrix[height-1][i] = img_energy[height-1][i];
    }

    for(int i = height-2; i>=0; i--){
        for(unsigned int j = 0; j<width; j++){
            if(j==0){
                dp_matrix[i][j] = img_energy[i][j] + min(dp_matrix[i+1][j], dp_matrix[i+1][j+1]);
            }else if(j==width-1){
                dp_matrix[i][j] = img_energy[i][j] + min(dp_matrix[i+1][j-1], dp_matrix[i+1][j]);
            }else{
                dp_matrix[i][j] = img_energy[i][j] + min(dp_matrix[i+1][j-1], min(dp_matrix[i+1][j], dp_matrix[i+1][j+1]));
            }
        }
    }
}

unsigned int* find_seam(unsigned int** dp_matrix, unsigned int height, unsigned int width){
    unsigned int* seam = new unsigned int[height];

    unsigned int min_val = dp_matrix[0][0];
    unsigned int start_col = 0;
    for (unsigned int j = 1; j < width; ++j) {
        if (dp_matrix[0][j] < min_val) {
            min_val = dp_matrix[0][j];
            start_col = j;
        }
    }
    seam[0] = start_col;
    for (unsigned int i=0; i<height-1; i++){
        unsigned int curr_col = seam[i];
        unsigned int next_col = curr_col;
        if(next_col>0 && dp_matrix[i+1][curr_col-1]<dp_matrix[i+1][next_col]){
            next_col = curr_col-1;
        }
        if(next_col<width-1 && dp_matrix[i+1][curr_col+1]<dp_matrix[i+1][next_col]){
            next_col = curr_col+1;
        }
        seam[i+1] = next_col;
    }
    return seam;
}

void draw_seam(Mat& img, unsigned int* seam){
    for (unsigned int i=0; i<img.rows; i++) {
        img.at<Vec3b>(i, seam[i]) = Vec3b(0, 0, 255);
    }
}

Mat remove_seam(const Mat& img, unsigned int* seam){
    int height = img.rows;
    int width = img.cols;
    Mat new_img(height, width - 1, img.type());

    for (int i = 0; i<height; i++) {
        unsigned int seam_col = seam[i];
        for (int j = 0; j<width-1; j++) {
            if (j < seam_col) {
                new_img.at<Vec3b>(i, j) = img.at<Vec3b>(i, j);
            } else {
                new_img.at<Vec3b>(i, j) = img.at<Vec3b>(i, j + 1);
            }
        }
    }
    return new_img;
}

void hdrMapGen(std::string outputPath) {
	const int width = 2400, height = 1200;

	float* image = new float[width * height * 3];

	std::cout << "Generate ENV :   0%";

	for (int v = 0; v < height; v++) {
		for (int u = 0; u < width; u++) {
			float
				y = cos((1.0 * v / height * PI)),
				x = sqrt(1 - y * y) * cos((u * 1.0 / width) * PI * 2),
				z = sqrt(1 - y * y) * sin((u * 1.0 / width) * PI * 2);

			for (int c = 0; c < 3; c++)
				image[(v * width + u) * 3 + c] = 0.0f;

			bool check;
			//pos X
			check = true;
			check = x >= 1 / sqrt(3);
			if (check) {
				float temp1 = y / fabs(x);
				float temp2 = z / fabs(x);
				check = temp1 <= 1.0f && temp1 >= -1.0f && temp2 <= 1.0f && temp2 >= -1.0f;
			}
			if (check)
				for (int c = 0; c < 3; c++)
					image[(v * width + u) * 3 + 0] = 1.0f;
			//image[(v * width + u) * 3 + 0] = x >= 0;
			image[(v * width + u) * 3 + 0] = 0.0;
			image[(v * width + u) * 3 + 1] = 0.0;
			image[(v * width + u) * 3 + 2] = 0.0;
			////neg X
			//check = true;
			//check = x <= -1 / sqrt(3);
			//if (check) {
			//	float temp1 = y / fabs(x);
			//	float temp2 = z / fabs(x);
			//	check = temp1 <= 1.0f && temp1 >= -1.0f && temp2 <= 1.0f && temp2 >= -1.0f;
			//}

			//if (check)
			//	for (int c = 0; c < 3; c++)
			//		image[(v * width + u) * 3 + 1] = 1.0f;


			////pos Y
			//check = true;
			//check = y >= 1 / sqrt(3);
			//if (check) {
			//	float temp1 = x / fabs(y);
			//	float temp2 = z / fabs(y);
			//	check = temp1 <= 1.0f && temp1 >= -1.0f && temp2 <= 1.0f && temp2 >= -1.0f;
			//}
			//if (check)
			//	for (int c = 0; c < 3; c++)
			//		image[(v * width + u) * 3 + 1] = 1.0f;
			////neg Y
			//check = true;
			//check = y <= -1 / sqrt(3);
			//if (check) {
			//	float temp1 = x / fabs(y);
			//	float temp2 = z / fabs(y);
			//	check = temp1 <= 1.0f && temp1 >= -1.0f && temp2 <= 1.0f && temp2 >= -1.0f;
			//}
			//if (check)
			//	for (int c = 0; c < 3; c++)
			//		image[(v * width + u) * 3 + 0] = 1.0f;


			////pos Z
			//check = true;
			//check = z >= 1 / sqrt(3);
			//if (check) {
			//	float temp1 = y / fabs(z);
			//	float temp2 = x / fabs(z);
			//	check = temp1 <= 1.0f && temp1 >= -1.0f && temp2 <= 1.0f && temp2 >= -1.0f;
			//}

			//if (check)
			//	for (int c = 0; c < 3; c++)
			//		image[(v * width + u) * 3 + 2] = 1.0f;

		}
		std::cout << "\b\b\b\b";
		std::cout.width(3);
		std::cout << v * 100 / (height - 1) << "%";
	}
	std::cout << std::endl;
	stbi_write_hdr(outputPath.c_str(), width, height, 3, image);

	delete[] image;
}
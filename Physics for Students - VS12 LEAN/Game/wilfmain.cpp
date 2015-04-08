//Wilf's Modifications of Thai-Duong Hoang and Kok-Lim Low's code to perform experiments...

//This file is included by main.cpp so that it has access to all the variables in
//main.cpp without having to build an extensive collection of externs...

struct ShaderSetting {
	bool usePoisson; bool useUpsampling; bool useTemporalSmoothing;
};

ShaderSetting computeShaderSetting(long levelCount) {
	ShaderSetting setting;
	setting.usePoisson = levelCount == 0;
	setting.useUpsampling = levelCount < (LEVEL_COUNT - 1);
	setting.useTemporalSmoothing = true;
	return setting;
}

void Draw() {
	frameRate.StartFrame();
	previousTime = currentTime;

	glPushMatrix();
	camera.ApplyCameraTransform();
	BuildMRTTextures();

	for (int index = 1, size = RESOLUTION / 2; index < LEVEL_COUNT; ++index, size /= 2) {
		Downsample(size, index);
	}
	for (int index = LEVEL_COUNT - 1, size = minResolution; index >= 0; --index, size *= 2) {
		Upsample(size, index);
	}
	glPopMatrix();

#if (DISABLE_TEMPORAL_BLENDING)
#else
	if (oddFrame)
		glBindTexture(GL_TEXTURE_RECTANGLE, aoTex[0]);
	else
		glBindTexture(GL_TEXTURE_RECTANGLE, lastFrameAOTex);
	oddFrame = !oddFrame;

	//Make a copy of the back buffer for temporal coherence...
	glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, RESOLUTION, RESOLUTION);
#endif //DISABLE_TEMPORAL_BLENDING

	glutSwapBuffers();
	frameRate.EndFrame();

	//Display the frame rate once every 30 frames...
	static int counter = 0; if (++counter > 30) {
		printf("%.2f fps\n", frameRate.GetLastFrameRate());
		counter = 0;
	}
}

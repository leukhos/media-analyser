# Macrium Software Coding Challenge (MP3 variant)

## Task Description

The task is to open an MP3 file and output some data on that file.

You should read an MP3 file specified on the command line and calculate the number of frames, the sample rate and average bitrate for the audio contained in the file.

- Your solution should handle constant bitrate (CBR) MP3s.
- Your solution should handle variable bitrate (VBR) MP3s.
- You should not parse ID3v1/ID3v2 structures (although you can if you want to) but you will need to ensure you do not accidentally interpret these as part of the MP3.
- We have provided bitrates for MPEG 1 Layer III (you may see files in other formats; for the purposes of this task we are only looking at MPEG 1 Layer III).
- Your solution should be concise. **Keep it simple!**

#### Error Handling

Your solutions should handle and report errors in a reasonable way, including missing files, data format errors and any other errors that prevent the tool from completing successfully.

## Starting Resources

To get you started, we have created a simple starter project for Visual Studio. VC++ Community Edition is available for free download here:

Visual Studio Download: <https://www.visualstudio.com/downloads/>

Visual Studio Project: [mp3tasks.zip](https://updates.macrium.com/jobs/coding/mp3task.zip)

In this project, we have included a sample function to get you started.

This task is not intended to take more than an evening or two - we'll accept partial answers or additional functionality if you'd like to provide it. Commenting the code to explain design and coding decisions or problem areas is helpful.

## Common Questions

- **Can I ask for help?** Of course! We are prepared to offer a limited number of hints and tips if you need them - feel free to contact us
- **Can I look things up on the internet?** Yes, of course you can. However, please do not copy solutions directly from the internet - we'll be asking you questions about your code.
- **Can I use AI/LLMs?** Yes, as with using the internet to help, we have no problems with using AI to assist with this task, however you will be expected to understand the code and discuss it during interview.
- **Do I have to use the Visual Studio project?** No, you can throw it away and start from scratch if you like, though it's helpful if you produce output in the same format.
- **What C++ standard should I use?** Internally we use C++17, but we will accept any version of C (ANSI C and above) or C++ (C++98 and above). Use whichever variant you are most comfortable working in.
- **Which libraries/functions should I use for IO/Strings/memory management?** Use whichever ones you'd like. We are more interested in your ability to code and solve problems than which specific libraries are used. C Standard Library functions (printf, malloc, char*), C++ (iostreams, std::wstring, new), Win32 (WriteFile, LocalAlloc) and any other method are all acceptable, so long as you can explain your reasoning.

Finally, we hope this will be an interesting task for you to undertake - enjoy!

**Macrium Software**

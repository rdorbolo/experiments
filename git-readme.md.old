# Experiments Repository

This repository is for learning and experimenting with Git commands.

## Git Terminology

- **Repository (repo)**: A storage space where your project lives, containing all files and complete history of changes
- **Clone**: Making a complete copy of a repository from GitHub to your local machine
- **Commit**: Saving a snapshot of your changes with a descriptive message
- **Push**: Uploading your local commits to the remote repository (GitHub)
- **Pull**: Downloading changes from the remote repository to your local machine

## Local Setup

This repository is cloned to: `~/experiments`

## Basic Git Workflow

### 1. Navigate to your repository
```bash
cd ~/experiments
```

### 2. Check status (see what's changed)
```bash
git status
```

### 3. Add files to staging (prepare for commit)
```bash
git add README.md
# Or add all changed files:
git add .
```

### 4. Commit changes (save snapshot)
```bash
git commit -m "Your descriptive message here"
```

### 5. Push to GitHub (upload changes)
```bash
git push
```

## Authentication Setup

GitHub requires a **Personal Access Token (PAT)** instead of your password.

### To create a PAT:
1. Go to GitHub.com → Settings → Developer settings
2. Select Personal access tokens → Tokens (classic)
3. Click "Generate new token (classic)"
4. Name it (e.g., "WSL Git Access")
5. Select scopes: minimum `repo` (full control of private repositories)
6. Generate and copy the token
7. Use this token as your password when Git prompts you

### Note on GPG vs PAT:
- **PAT** = Authentication (like a password to access GitHub)
- **GPG Keys** = Signing commits (optional verification feature)

## Common Commands

- `git status` - Check what files have changed
- `git add <file>` - Stage specific file
- `git add .` - Stage all changed files
- `git commit -m "message"` - Commit staged changes
- `git push` - Push commits to GitHub
- `git pull` - Pull latest changes from GitHub
- `git log` - View commit history
- `git restore <file>` - Discard changes in working directory
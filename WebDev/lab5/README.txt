INSURANCE CLAIM MANAGEMENT SYSTEM

------------------------------------------------------
SETUP INSTRUCTIONS
------------------------------------------------------

1. Requirements:
   - PHP 7.4 or higher installed.
   - A web browser.

2. Running the Application locally:
   Open a terminal in this directory (/home/mtech/Documents/206125030/WebDev/lab5) and run:
   
       php -S localhost:8000

   Then open your browser and visit: http://localhost:8000

------------------------------------------------------
FEATURES IMPLEMENTED
------------------------------------------------------
1. Signup Page:
   - Registers users with validation.
   - Stores user details in 'users.json' for persistence.
   - Also mirrors storage in PHP Session as requested.

2. Login Page:
   - Validates credentials from stored users.
   - "Remember Me" checkbox uses Cookies to auto-fill username next time.

3. Dashboard:
   - Protected area (requires login).
   - "Claim Insurance" starts the multi-step form.
   - "Resume Application" appears if a previous step was saved (via Cookie).

4. Multi-step Claim Form:
   - Step 1: Personal Details (saved in Session).
   - Step 2: Incident Details (saved in Session).
   - Progress saved in Cookie (allows resuming even after browser restart if session is active or just navigation).

5. Final Summary:
   - Reviews all details.
   - "Confirm & Submit" simulates submission.
   - Displays final success message.

6. Session Management:
   - Active session required for all internal pages.
   - Logout destroys session and redirects to login.

7. Forgotten Password:
   - Simulated password reset flow.

------------------------------------------------------
NOTES
------------------------------------------------------
- The "database" is a JSON file (users.json) created automatically upon first registration.
- Passwords are stored in plain text as per requirements.

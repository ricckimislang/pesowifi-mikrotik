/**
 * MikroTik Timer Management System
 * Gets remaining time from MikroTik session variables
 * Updates progress bar and text display
 */

class MikrotikTimerManager {
    constructor() {
        this.remainingTimer = null;
        this.sessionTime = "$(session-time-left-secs)";
        this.currentVoucher = "$(username)";
        this.initialTime = 0;
        this.currentTime = 0;
        this.warningThreshold = 300; // 5 minutes
        this.dangerThreshold = 60;   // 1 minute
        this.isExpired = false;
        
        // Debug: Log the raw session time value
        console.log('Raw session time variable:', this.sessionTime);
        
        this.init();
    }

    init() {
        this.updateProgressBar();
        this.startCountdown();
        this.updateConnectionStatus();
    }

    startCountdown() {
        // Try to get session time from multiple sources
        let time = parseInt(this.sessionTime);
        
        // Debug: Check if MikroTik variable was processed
        console.log('Parsed session time:', time);
        console.log('Is NaN:', isNaN(time));
        
        // If MikroTik variable wasn't processed, try to get from global variables
        if (isNaN(time) || this.sessionTime.includes('$(session-time-left-secs)')) {
            console.log('MikroTik variable not processed, checking global variables...');
            
            // Try to get from window.sessiontime if available
            if (typeof window.sessiontime !== 'undefined') {
                time = parseInt(window.sessiontime);
                console.log('Found sessiontime in window:', window.sessiontime);
            }
            
            // Try to get from document ready script
            if (isNaN(time) && typeof sessiontime !== 'undefined') {
                time = parseInt(sessiontime);
                console.log('Found sessiontime in global:', sessiontime);
            }
        }
        
        // Final fallback - show demo time for testing
        if (isNaN(time) || time < 0) {
            console.log('No valid session time found, using demo time');
            time = 3600; // 1 hour for demo
            this.showDebugInfo();
        }
        
        if (time === 0) {
            this.displayUnlimited();
            return;
        }

        console.log('Starting countdown with time:', time);
        this.initialTime = time;
        this.currentTime = time;
        this.updateDisplay(time);
        
        this.remainingTimer = setInterval(() => {
            time--;
            this.currentTime = time;
            this.updateDisplay(time);
            
            if (time <= 0) {
                this.handleTimeExpired();
            }
        }, 1000);
    }

    updateDisplay(seconds) {
        const progressFill = document.getElementById('timeProgressFill');
        const progressPercentage = document.getElementById('progressPercentage');
        const timeDisplay = document.getElementById('remainingTimeText');
        const statusDot = document.getElementById('statusDot');
        const statusText = document.getElementById('connectionStatus');
        
        if (!progressFill || !progressPercentage || !timeDisplay) return;

        // Calculate and update progress
        const progress = this.initialTime > 0 ? (seconds / this.initialTime) * 100 : 0;
        progressFill.style.width = progress + '%';
        progressPercentage.textContent = Math.round(progress) + '%';

        // Format time display
        const formattedTime = this.formatTime(seconds);
        timeDisplay.textContent = formattedTime;

        // Apply warning/danger states
        this.applyTimerState(seconds, progressFill, timeDisplay, statusDot, statusText);
        
        // Update connection status based on time
        this.updateConnectionStatusByTime(seconds);
    }

    formatTime(seconds) {
        const days = Math.floor(seconds / (3600 * 24));
        const hours = Math.floor((seconds % (3600 * 24)) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;

        let timeString = '';
        
        if (days > 0) {
            timeString += days + 'd ';
        }
        
        timeString += String(hours).padStart(2, '0') + ':';
        timeString += String(minutes).padStart(2, '0') + ':';
        timeString += String(secs).padStart(2, '0');
        
        return timeString;
    }

    applyTimerState(seconds, progressElement, timeElement, statusDot, statusText) {
        // Remove all state classes
        progressElement.classList.remove('warning', 'danger', 'expired');
        timeElement.classList.remove('warning', 'danger', 'expired');
        
        if (seconds <= 0) {
            progressElement.classList.add('expired');
            timeElement.classList.add('expired');
            statusDot.classList.add('disconnected');
            statusText.textContent = 'Expired';
            statusText.style.color = '#ff4757';
        } else if (seconds <= this.dangerThreshold) {
            progressElement.classList.add('danger');
            timeElement.classList.add('danger');
            // Keep connection status as "Connected" but update time color
            this.updateConnectionStatus(seconds);
        } else if (seconds <= this.warningThreshold) {
            progressElement.classList.add('warning');
            timeElement.classList.add('warning');
            // Keep connection status as "Connected" but update time color
            this.updateConnectionStatus(seconds);
        } else {
            // Normal state - keep connection as "Connected"
            this.updateConnectionStatus(seconds);
        }
    }

    updateConnectionStatus(seconds = null) {
        const statusDot = document.getElementById('statusDot');
        const statusText = document.getElementById('connectionStatus');
        
        if (statusDot && statusText) {
            // Use provided seconds or current time
            const time = seconds !== null ? seconds : (this.currentTime || parseInt(this.sessionTime));
            console.log('Updating connection status with time:', time);
            
            if (time > 0) {
                statusDot.classList.remove('disconnected');
                statusText.textContent = 'Connected';
                statusText.style.color = '#4cd137';
            } else if (time === 0) {
                statusDot.classList.remove('disconnected');
                statusText.textContent = 'Unlimited';
                statusText.style.color = '#05c46b';
            } else {
                statusDot.classList.add('disconnected');
                statusText.textContent = 'No Session';
                statusText.style.color = '#eb2f06';
            }
        }
    }

    showDebugInfo() {
        // Show debug information on the page
        const timeDisplay = document.getElementById('remainingTimeText');
        if (timeDisplay) {
            timeDisplay.style.color = '#ffc312';
            timeDisplay.textContent = 'DEMO: 01:00:00';
        }
        
        const statusText = document.getElementById('connectionStatus');
        if (statusText) {
            statusText.textContent = 'Demo Mode';
            statusText.style.color = '#ffc312';
        }
        
        console.log('=== DEBUG INFO ===');
        console.log('Session variable:', this.sessionTime);
        console.log('Current time:', this.currentTime);
        console.log('Window sessiontime:', typeof window.sessiontime !== 'undefined' ? window.sessiontime : 'undefined');
        console.log('Global sessiontime:', typeof sessiontime !== 'undefined' ? sessiontime : 'undefined');
        console.log('================');
    }

    updateConnectionStatusByTime(seconds) {
        const statusDot = document.getElementById('statusDot');
        const statusText = document.getElementById('connectionStatus');
        
        if (seconds <= 0) {
            statusDot.classList.add('disconnected');
            statusText.textContent = 'Expired';
            statusText.style.color = '#ff4757';
        }
    }

    updateProgressBar() {
        const progressFill = document.getElementById('timeProgressFill');
        const progressPercentage = document.getElementById('progressPercentage');
        
        if (progressFill && progressPercentage) {
            // Initialize with current session time
            const time = parseInt(this.sessionTime);
            if (time > 0) {
                progressFill.style.width = '100%';
                progressPercentage.textContent = '100%';
            } else {
                progressFill.style.width = '0%';
                progressPercentage.textContent = '0%';
                progressFill.classList.add('expired');
            }
        }
    }

    displayUnlimited() {
        const progressFill = document.getElementById('timeProgressFill');
        const progressPercentage = document.getElementById('progressPercentage');
        const timeDisplay = document.getElementById('remainingTimeText');
        const statusDot = document.getElementById('statusDot');
        const statusText = document.getElementById('connectionStatus');
        
        if (progressFill) {
            progressFill.style.width = '100%';
            progressFill.style.background = 'linear-gradient(90deg, #05c46b, #00d2d3)';
        }
        
        if (progressPercentage) {
            progressPercentage.textContent = 'Unlimited';
        }
        
        if (timeDisplay) {
            timeDisplay.textContent = 'Unlimited';
            timeDisplay.style.color = '#05c46b';
        }
        
        if (statusDot && statusText) {
            statusDot.classList.remove('disconnected');
            statusText.textContent = 'Connected';
            statusText.style.color = '#4cd137';
        }
    }

    handleTimeExpired() {
        if (this.isExpired) return;
        this.isExpired = true;
        
        clearInterval(this.remainingTimer);
        
        // Show expiration notification
        this.showNotification('Time Expired', 'Your session has expired. You will be logged out shortly.', 'warning');
        
        // Auto logout after delay
        setTimeout(() => {
            this.logout();
        }, 5000);
    }

    showNotification(title, message, type = 'info') {
        // Try to use toast if available, otherwise use alert
        if (typeof $ !== 'undefined' && $.toast) {
            $.toast({
                title: title,
                content: message,
                type: type,
                delay: 5000
            });
        } else {
            // Fallback to custom notification
            this.showCustomNotification(title, message, type);
        }
    }

    showCustomNotification(title, message, type) {
        // Create notification element
        const notification = document.createElement('div');
        notification.className = `timer-notification timer-notification-${type}`;
        notification.innerHTML = `
            <div class="notification-title">${title}</div>
            <div class="notification-message">${message}</div>
        `;
        
        // Add styles
        notification.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            background: ${type === 'warning' ? '#eb2f06' : '#e94560'};
            color: white;
            padding: 15px 20px;
            border-radius: 8px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.3);
            z-index: 9999;
            max-width: 300px;
            transform: translateX(400px);
            transition: transform 0.3s ease;
        `;
        
        document.body.appendChild(notification);
        
        // Animate in
        setTimeout(() => {
            notification.style.transform = 'translateX(0)';
        }, 100);
        
        // Remove after delay
        setTimeout(() => {
            notification.style.transform = 'translateX(400px)';
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 300);
        }, 5000);
    }

    logout() {
        // Try to submit logout form using explicit ID
        const logoutForm = document.getElementById('logoutForm') || document.logout || document.forms['logout'];
        if (logoutForm) {
            logoutForm.submit();
        } else {
            // Fallback: redirect to logout URL
            const logoutUrl = "$(link-logout)";
            if (logoutUrl && logoutUrl !== "$(link-logout)") {
                window.location.href = logoutUrl;
            } else {
                // Last resort: reload page
                window.location.reload();
            }
        }
    }

    destroy() {
        if (this.remainingTimer) {
            clearInterval(this.remainingTimer);
            this.remainingTimer = null;
        }
    }

    // Method to update time if session time changes (e.g., after extending time)
    updateSessionTime(newSessionTime) {
        const time = parseInt(newSessionTime);
        if (time === 0 || isNaN(time)) {
            this.displayUnlimited();
            return;
        }

        // Clear existing timer
        if (this.remainingTimer) {
            clearInterval(this.remainingTimer);
        }

        // Update session time and restart countdown
        this.sessionTime = newSessionTime;
        this.currentTime = time;
        this.initialTime = time;
        this.isExpired = false;
        
        this.updateDisplay(time);
        this.startCountdown();
    }

    // Get current remaining time
    getCurrentTime() {
        return this.currentTime;
    }

    // Get initial time
    getInitialTime() {
        return this.initialTime;
    }
}

// Initialize timer when DOM is ready and after existing scripts
document.addEventListener('DOMContentLoaded', function() {
    // Wait longer for MikroTik variables to be processed by existing scripts
    setTimeout(() => {
        console.log('Initializing MikrotikTimerManager...');
        window.mikrotikTimer = new MikrotikTimerManager();
    }, 500);
});

// Also try to initialize after jQuery is ready
$(document).ready(function() {
    setTimeout(() => {
        if (!window.mikrotikTimer) {
            console.log('Initializing MikrotikTimerManager from jQuery ready...');
            window.mikrotikTimer = new MikrotikTimerManager();
        }
    }, 1000);
});

// Clean up on page unload
window.addEventListener('beforeunload', function() {
    if (window.mikrotikTimer) {
        window.mikrotikTimer.destroy();
    }
});

// Global function to update timer (can be called from other scripts)
window.updateTimerSession = function(newSessionTime) {
    if (window.mikrotikTimer) {
        window.mikrotikTimer.updateSessionTime(newSessionTime);
    }
};

let currentProofTree = null;

function showError(message) {
    const errorDiv = document.getElementById('error-display');
    errorDiv.textContent = message;
    errorDiv.classList.remove('hidden');
    setTimeout(() => {
        errorDiv.classList.add('hidden');
    }, 5000);
}

function hideError() {
    document.getElementById('error-display').classList.add('hidden');
}

function showLoading(containerId) {
    const container = document.getElementById(containerId);
    container.innerHTML = '<div class="loading">Loading</div>';
}

async function explainQuery() {
    const query = document.getElementById('query-input').value.trim();
    if (!query) {
        showError('Please enter a query');
        return;
    }
    
    hideError();
    showLoading('tree-display');
    
    try {
        const response = await fetch(`/api/explain?tuple=${encodeURIComponent(query)}`);
        const data = await response.json();
        
        if (data.error) {
            showError(data.error);
            document.getElementById('tree-display').innerHTML = '';
        } else {
            currentProofTree = data.proof;
            renderProofTree(data.proof, 'tree-display');
        }
    } catch (error) {
        showError('Failed to fetch explanation: ' + error.message);
        document.getElementById('tree-display').innerHTML = '';
    }
}


function renderProofTree(node, containerId) {
    const container = document.getElementById(containerId);
    container.innerHTML = '';
    
    if (!node) {
        container.innerHTML = '<p>No proof tree available</p>';
        return;
    }
    
    const treeElement = createTreeElement(node);
    container.appendChild(treeElement);
}

function createTreeElement(node, depth = 0) {
    const nodeDiv = document.createElement('div');
    nodeDiv.className = 'tree-node';
    nodeDiv.style.marginLeft = (depth * 20) + 'px';
    
    const contentDiv = document.createElement('div');
    contentDiv.className = 'tree-node-content';
    
    let hasChildren = node.children && node.children.length > 0;
    
    if (hasChildren) {
        contentDiv.classList.add('expandable');
        const toggle = document.createElement('span');
        toggle.className = 'tree-node-toggle';
        toggle.textContent = '▶ ';
        contentDiv.appendChild(toggle);
        
        contentDiv.addEventListener('click', (e) => {
            e.stopPropagation();
            toggleNode(nodeDiv, node, depth);
        });
    }
    
    const textSpan = document.createElement('span');
    // Handle Souffle proof tree structure
    let displayText = '';
    if (node.premises) {
        displayText = node.premises;
        if (node['rule-number']) {
            displayText += ' ' + node['rule-number'];
        }
    } else if (node.axiom) {
        displayText = '📄 ' + node.axiom + ' (fact)';
    } else {
        displayText = node.text || node.label || 'Unknown node';
    }
    textSpan.textContent = displayText;
    contentDiv.appendChild(textSpan);
    
    nodeDiv.appendChild(contentDiv);
    
    if (hasChildren) {
        const childrenDiv = document.createElement('div');
        childrenDiv.className = 'tree-node-children hidden';
        nodeDiv.appendChild(childrenDiv);
    }
    
    return nodeDiv;
}

function toggleNode(nodeDiv, node, depth) {
    const toggle = nodeDiv.querySelector('.tree-node-toggle');
    const childrenDiv = nodeDiv.querySelector('.tree-node-children');
    
    if (!childrenDiv) return;
    
    if (childrenDiv.classList.contains('hidden')) {
        // Expand
        toggle.textContent = '▼ ';
        childrenDiv.classList.remove('hidden');
        nodeDiv.classList.add('expanded');
        
        // Render children if not already rendered
        if (childrenDiv.children.length === 0 && node.children) {
            node.children.forEach(child => {
                const childElement = createTreeElement(child, depth + 1);
                childrenDiv.appendChild(childElement);
            });
        }
    } else {
        // Collapse
        toggle.textContent = '▶ ';
        childrenDiv.classList.add('hidden');
        nodeDiv.classList.remove('expanded');
    }
}

// Handle Enter key in input fields
document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('query-input').addEventListener('keypress', function(e) {
        if (e.key === 'Enter') {
            explainQuery();
        }
    });
});